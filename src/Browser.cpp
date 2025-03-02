#include "Browser.h"

#include <cctype>
#include <clocale>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <fstream>
#include <csignal>
#include <algorithm>
#include <array>
#include <string>
#include <unistd.h>
#include <unordered_map>

#include <ncurses.h>

#include "Console.h"
#include "RtypesCore.h"
#include "TTree.h"
#include "TTreeFormula.h"
#include "TH1.h"
#include "TH2.h"

#include "AxisTicks.h"
#include "RootFile.h"
#include "definitions.h"
#include "nlohmann/json.hpp"

volatile bool resize_flag = false;

FileBrowser::FileBrowser() {
    initNcurses();
    colorWindow.init();
    loadSettings();
    console.loadCommandHistory(dotpath / "tbhistory");
    initAllWindows();

    refresh();
    box(dir_window, 0, 0);
    box(main_window, 0, 0);
    wrefresh(main_window);

    refreshCMDWindow();

    drawEssentials();
}

void FileBrowser::initAllWindows() {
    const int sizex = getmaxx(stdscr);
    const int sizey = getmaxy(stdscr);
    createWindow(main_window, sizey - bottom_height, sizex - menu_width - yaxis_spacing, 1, menu_width + yaxis_spacing);
    createWindow(dir_window, sizey - bottom_height, menu_width, 1, 0);
    createWindow(cmd_window, 3, sizex - 25, sizey - bottom_height + 3, 20 + 5);
}

FileBrowser::~FileBrowser() {
    saveSettings();
    delwin(cmd_window);
    delwin(main_window);
    delwin(dir_window);
    endwin();
}

void FileBrowser::initNcurses() {
    // Init NCURSES
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);

    mouseinterval(0);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    start_color();
    init_pair(TermColor::col_blue, COLOR_BLUE, COLOR_BLACK);
    init_pair(TermColor::col_green, COLOR_GREEN, COLOR_BLACK);
    init_pair(TermColor::col_red, COLOR_RED, COLOR_BLACK);
    init_pair(TermColor::col_white, COLOR_WHITE, COLOR_BLACK);
    init_pair(TermColor::col_yellow, COLOR_YELLOW, COLOR_BLACK);
    init_pair(TermColor::col_win_bkg, COLOR_WHITE, 237);
    init_pair(TermColor::col_whiteblue, 33, COLOR_BLACK);
    init_pair(TermColor::col_start_palette, 1, COLOR_BLACK);

    // initialize grayscale
    int grayscale = col_grayscale_start;
    init_pair(grayscale++, 16, COLOR_BLACK); // True black
    for (int c = 232; c <= 255; ++c) {
        init_pair(grayscale++, c, COLOR_BLACK);
    }
    init_pair(grayscale++, 15, COLOR_BLACK); // True white

    for (int c = col_start_palette, col256 = 17; col256 < 231; ++col256) {
        init_pair(c++, col256, COLOR_BLACK);
    }
}

void FileBrowser::loadSettings() {
    namespace fs = std::filesystem;

    dotpath = fs::path(std::getenv("HOME")) / ".tbrowser";
    // Make sure dotfile exists
    if (!fs::exists(dotpath / "settings.json")) {
        fs::create_directory(dotpath);
        std::ofstream settings(dotpath / "settings.json");
        if (settings.is_open()) {
            settings << R"({"blockmode":"2x2","statsbox":true,"menu_width":20})";
        }
        else {
            std::cerr << "Could not create user settings file" << std::endl;
            return;
        }
        settings.close();
    }

    // Load settings
    std::ifstream settings(dotpath / "settings.json");
    if (settings.is_open()) {
        settings >> settings_json;
        if (settings_json.contains("blockmode")) {
            if (settings_json["blockmode"] == "2x2") {      blockmode = 2; }
            else if (settings_json["blockmode"] == "3x2") { blockmode = 3; }
            else if (settings_json["blockmode"] == "4x2") { blockmode = 4; }
            else { blockmode = 2; };
        }
        if (settings_json.contains("statsbox")) {
            showstats = settings_json["statsbox"];
        }
        if (settings_json.contains("menu_width") && settings_json["menu_width"].is_number()) {
            menu_width = settings_json["menu_width"];
        }
    }
}

void FileBrowser::saveSettings() {
    std::ofstream saveSettings(dotpath / "settings.json", std::ios::out);
    if (saveSettings.is_open()) {
        switch (blockmode) {
            case 2: settings_json["blockmode"] = "2x2"; break;
            case 3: settings_json["blockmode"] = "3x2"; break;
            case 4: settings_json["blockmode"] = "4x2"; break;
        }
        settings_json["statsbox"] = showstats;
        settings_json["menu_width"] = menu_width;
        saveSettings << settings_json;
        saveSettings.close();
    }
}

void FileBrowser::loadFile(std::string filename) {
    mvwprintw(dir_window, 1, 1, "Reading...");
    wrefresh(dir_window);

    root_file.load(filename);
    console.setTabCompletionDict(root_file.displayList);
}

void FileBrowser::printDirectories() {
    if (skipDraw) { skipDraw = false; return; }
    int x = getbegx(dir_window) + 1;
    int y = getbegy(dir_window) + 1;
    int maxlines = getmaxy(dir_window) - 2;

    if (searchMode.isActive) {
        attron(A_BOLD | A_ITALIC);
        mvprintw(0, 0, "/%s", searchMode.input.c_str());
        attroff(A_BOLD | A_ITALIC);
        clrtoeol();
    }

    int entry = -menu_scroll_pos;
    auto print_entry = [this, &entry, y, x](std::string name, RootFile::Node* node){
        std::string entry_label;
        name = std::string(node->nesting * 2, ' ') + name;
        TermColor col = col_white;
        int attr = A_NORMAL;
        switch (node->type) {
            case NodeType::DIRECTORY:
                if (node->openState & RootFile::Node::DIR_OPEN) 
                    { entry_label = fmtstring("{} {}", SYMB_FOLDER_OPEN, name); }
                else
                    { entry_label = fmtstring("{} {}", SYMB_FOLDER_CLOSED, name); }
                col = col_yellow;
                break;
            case NodeType::TLEAF:   entry_label = fmtstring("{} {}", SYMB_TLEAF, name); attr = A_ITALIC; break;
            case NodeType::TTREE:   entry_label = fmtstring("{} {}", SYMB_TTREE, name); col = col_green; attr = A_BOLD; break;
            case NodeType::HIST:    entry_label = fmtstring("{} {}", SYMB_THIST, name); col = col_blue; attr = A_ITALIC; break;
            case NodeType::UNKNOWN: entry_label = fmtstring("{} {}", SYMB_TUNKNOWN, name); col = col_red; break;
        }
        attron(attr | COLOR_PAIR(col));
        if (entry == selected_pos) {
            attron(A_REVERSE | A_ITALIC);
        }
        auto entryfmt = fmtstring("%.{}s", menu_width);
        mvprintw(y + entry, x, entryfmt.c_str(), entry_label.c_str());
        if (entry == selected_pos) {
            attroff(A_REVERSE | A_ITALIC);
        }
        attroff(attr | COLOR_PAIR(col));
        entry++;
    };

    for (const auto& [name, node] : root_file.displayList) {
        if (entry < 0) {
            entry++;
        }
        else if (entry < maxlines) {
            bool showCondition = (node->openState & RootFile::Node::LISTED) != 0;
            if (searchMode.isActive) {
                showCondition = node->showInSearch;
            }

            if (showCondition) {
                print_entry(name, node);
                if (entry == selected_pos+1) {
                    auto descr = root_file.toString(node);
                    move(getmaxy(stdscr)-1, 0);
                    clrtoeol();
                    mvprintw(getmaxy(stdscr)-1, 0, "%s", descr.c_str());
                }
            }
        }
    }

    box(dir_window, 0, 0);
    if (searchMode.isActive) {
        attron(A_BOLD | A_REVERSE);
        mvprintw(y - 1, x, "[SEARCH MODE]");
        mvprintw(y + maxlines, x, "[EXIT WITH '/']");
        attroff(A_BOLD | A_REVERSE);
    }

    wrefresh(dir_window);
    // Very important to refresh(), otherwise input appears delayed and program
    // looks glitched while resize
    refresh();
}

void FileBrowser::selectionDown() {
    if (selected_pos == getmaxy(dir_window) - 3) {
        menu_scroll_pos++;
    }
    else {
        selected_pos = std::min<int>(std::min<int>(root_file.menuLength(searchMode.isActive), getmaxy(dir_window) - 3), selected_pos + 1); 
    }
}

void FileBrowser::selectionUp() {
    if (selected_pos == 0 && menu_scroll_pos > 0) {
        menu_scroll_pos--;
    }
    selected_pos = std::max<int>(0, selected_pos - 1); 
}

void FileBrowser::goTop() {
    selected_pos = 0; 
    menu_scroll_pos = 0; 
}

void FileBrowser::goBottom() {
    selected_pos = getmaxy(dir_window) - 3;
    menu_scroll_pos = root_file.menuLength(searchMode.isActive) - getmaxy(dir_window) + 2;
}

void FileBrowser::toggleStatsBox() {
    showstats = !showstats; 
} 

void FileBrowser::toggleLogy() {
    logscale = !logscale; 
} 

int FileBrowser::getBinsx() const { 
    return 2 * mainwin_x - 4; 
}

int FileBrowser::getBinsy() const { 
    return blockmode * mainwin_y - 2 * blockmode;
}

void FileBrowser::toggleBlockMode() {
    blockmode++;
    if (blockmode >= 5) blockmode = 2;
    attron(A_BOLD);
    mvprintw(0, 0, "%ix2 char mode", blockmode);
    attroff(A_BOLD);
    clrtoeol();
}

TTree* FileBrowser::getActiveTTree() {
    // Get selected tree or the tree of the currently selected branch
    TTree* ttree = nullptr;
    std::size_t entry = selected_pos + menu_scroll_pos;
    if (entry < root_file.displayList.size()) {
        auto [name, node] = root_file.displayList.at(entry);
        if (node->type == NodeType::TLEAF) {
            ttree = root_file.m_trees[node->mother->index];
        }
        else if (node->type == NodeType::TTREE) {
            ttree = root_file.m_trees[node->index];
        }
    }

    if (ttree == nullptr) {
        // No tree selected, return obvious single choice
        if (root_file.m_trees.size() == 1) {
            return root_file.m_trees[0];
        }
        console.setError("No TTree found in file!");
    }

    return ttree;
}


void FileBrowser::plotHistogram() {
    if (console.hasCommand()) {
        if (std::get<0>(console.current_args).hist2d) {
            plot2DHistogram(console.current_args);
        }
        else {
            plotHistogram(console.current_args);
        }
    }
    else {
        std::size_t entry = selected_pos + menu_scroll_pos;
        if (entry < root_file.displayList.size()) {
            auto [name, node] = root_file.displayList.at(entry);
            if (node->type == NodeType::TLEAF) {
                plotHistogram(root_file.m_trees.at(node->mother->index), 
                        root_file.m_leaves.at(node->index));
            }
        }
    }
}

bool FileBrowser::isRunning() const {
    return is_running;
}

void FileBrowser::plotHistogram(TTree* tree, TLeaf* leaf) {
    // Get window position and size
    const int winx = getbegx(main_window);
    const int winy = getbegy(main_window);
    getmaxyx(main_window, mainwin_y, mainwin_x);
    box(main_window, 0, 0);

    mvprintw(winy + mainwin_y / 2, winx + mainwin_x / 2 - 5, "Reading %s...", leaf->GetName());
    refresh();

    const char* leafname = leaf->GetName();

    // Get bounds
    auto bins_x = getBinsx();
    auto bins_y = getBinsy();
    auto min = tree->GetMinimum(leafname);
    auto max = tree->GetMaximum(leafname);
    if (min == max) {
        // Handle single value histograms
        min--;
        max++;
    }
    AxisTicks xaxis(min, max);

    TH1D hist("H", leafname, bins_x, xaxis.minAdjusted(), xaxis.maxAdjusted());

    tree->Project("H", leafname);

    AxisTicks yaxis(0, hist.GetAt(hist.GetMaximumBin())*top_hist_clear, 5, logscale);
    
    plotYAxis(yaxis, true);
    plotXAxis(xaxis, false); // use minAdjusted as in histogram
    plotASCIIHistogram(&hist, bins_y, bins_x);
    plotCanvasAnnotations(&hist);

    refresh();
}

void FileBrowser::plotHistogram(const Console::DrawArgs& args) {
    // Get window position and size
    int winx = getbegx(main_window);
    int winy = getbegy(main_window);
    getmaxyx(main_window, mainwin_y, mainwin_x);
    box(main_window, 0, 0);

    mvprintw(winy + mainwin_y / 2, winx + mainwin_x / 2 - 5, "Reading...");
    refresh();

    const auto& [varexp, selection, option, nentries, firstentry] = args;

    // Get selected tree
    TTree* ttree = getActiveTTree();
    if (ttree == nullptr) {
        return;
    }

    // Get bounds
    auto bins_x = getBinsx();
    auto bins_y = getBinsy();

    ttree->SetEstimate(ttree->GetEntries());
    Long64_t entriesDrawn = -1;
    try {
        entriesDrawn = ttree->Draw(varexp.expression.c_str(), selection.c_str(), option, nentries, firstentry);
    }
    catch (...) {
        console.setError("TTreeFormula Error");
        return;
    }

    auto vector_branches = [ttree] (const char* form) -> TLeaf* {
        // Check if formula involves vector branches
        TTreeFormula formula("FORM", form, ttree);
        for (int v = 0; v < formula.GetNdim(); ++v) {
            if (TLeaf* leaf = formula.GetLeaf(v)->GetLeafCount(); leaf != nullptr) {
                formula.Delete();
                return leaf;
            }
        }
        formula.Delete();
        return nullptr;
    };

    if (TLeaf* lenLeaf = vector_branches(varexp.expression.c_str()); lenLeaf != nullptr && false) {
        // AFAIK, an efficient loop is required now
        // SKIP!
        TTreeFormula formula("FORM", varexp.expression.c_str(), ttree);
        auto nEntriesTop = ttree->GetEntries();
        Long64_t len;
        ttree->SetBranchStatus(lenLeaf->GetName(), 1);
        ttree->SetBranchAddress(lenLeaf->GetName(), &len);
        for (int i = 0; i < nEntriesTop; ++i) {
            ttree->GetEntry(i);

        }
    }
    else if (entriesDrawn != -1) {
        Double_t* data = ttree->GetV1();
        // This is a hack
        Long64_t n = std::min<Long64_t>(ttree->GetSelectedRows(), ttree->GetEntries());
        Double_t min = *std::min_element(data, data + n);
        Double_t max = *std::max_element(data, data + n);

        std::string title;
        if (selection.empty()) {
            title = varexp.expression;
        }
        else {
            title = fmtstring("{} ({})", varexp.expression, selection);
        }

        TH1D hist;
        AxisTicks xaxis;
        if (!varexp.limits.empty()) {
            min = varexp.limits.at(0);
            max = varexp.limits.at(1);
            xaxis = AxisTicks(min, max, 10);
            hist = TH1D("TEMP", title.c_str(), bins_x, min, max);
            for (int i = 0; i < n; ++i) {
                if (data[i] >= min && data[i] <= max) {
                    hist.Fill(data[i]);
                }
            }
            hist.Draw("goff");
        }
        else {
            xaxis = AxisTicks(min, max, 10);
            hist = TH1D("TEMP", title.c_str(), bins_x, xaxis.minAdjusted(), xaxis.maxAdjusted());
            hist.FillN(n, data, nullptr);
            hist.Draw("goff");
        }

        if (min == max) {
            // Handle single value histograms
            min--;
            max++;
        }
        AxisTicks yaxis(0, hist.GetAt(hist.GetMaximumBin())*top_hist_clear, 5, logscale);

        plotYAxis(yaxis, true);
        plotXAxis(xaxis, !varexp.limits.empty());
        plotASCIIHistogram(&hist, bins_y, bins_x);
        plotCanvasAnnotations(&hist);
    }
    else {
        console.setError("Branch not found");
    }

    refresh();
}

void FileBrowser::plot2DHistogram(const Console::DrawArgs& args) {
    // The console has already checked whether the format is correct for 2D drawing
    // Get window position and size
    int winx = getbegx(main_window);
    int winy = getbegy(main_window);
    getmaxyx(main_window, mainwin_y, mainwin_x);
    box(main_window, 0, 0);

    mvprintw(winy + mainwin_y / 2, winx + mainwin_x / 2 - 5, "Reading...");
    refresh();

    auto& [varexp, selection, option, nentries, firstentry] = args;

    TTree* ttree = getActiveTTree();
    if (ttree == nullptr) {
        return;
    }

    // Get bounds
    auto bins_x = mainwin_x - 2;
    auto bins_y = mainwin_y - 2;

    ttree->SetEstimate(ttree->GetEntries());
    Long64_t entriesDrawn = -1;
    try {
        entriesDrawn = ttree->Draw(varexp.expression.c_str(), selection.c_str(), option, nentries, firstentry);
    }
    catch (...) {
        console.setError("TTreeFormula Error");
        return;
    }

    auto vector_branches = [ttree] (const char* form) -> TLeaf* {
        // Check if formula involves vector branches
        TTreeFormula formula("FORM", form, ttree);
        for (int v = 0; v < formula.GetNdim(); ++v) {
            if (TLeaf* leaf = formula.GetLeaf(v)->GetLeafCount(); leaf != nullptr) {
                formula.Delete();
                return leaf;
            }
        }
        formula.Delete();
        return nullptr;
    };

    if (TLeaf* lenLeaf = vector_branches(varexp.expression.c_str()); lenLeaf != nullptr && false) {
        // AFAIK, an efficient loop is required now
        // SKIP!
        TTreeFormula formula("FORM", varexp.expression.c_str(), ttree);
        auto nEntriesTop = ttree->GetEntries();
        Long64_t len;
        ttree->SetBranchStatus(lenLeaf->GetName(), 1);
        ttree->SetBranchAddress(lenLeaf->GetName(), &len);
        for (int i = 0; i < nEntriesTop; ++i) {
            ttree->GetEntry(i);

        }
    }
    else if (entriesDrawn != -1) {
        Double_t* data_y = ttree->GetV1();
        Double_t* data_x = ttree->GetV2();
        // This is a hack
        Long64_t n = std::min<Long64_t>(ttree->GetSelectedRows(), ttree->GetEntries());
        Double_t minx = *std::min_element(data_x, data_x + n);
        Double_t miny = *std::min_element(data_y, data_y + n);
        Double_t maxx = *std::max_element(data_x, data_x + n);
        Double_t maxy = *std::max_element(data_y, data_y + n);

        std::string title;
        if (selection.empty()) {
            title = varexp.expression;
        }
        else {
            title = fmtstring("{} ({})", varexp.expression, selection);
        }

        TH2D hist2d;
        if (!varexp.limits.empty()) {
            minx = varexp.limits.at(0);
            maxx = varexp.limits.at(1);
            miny = varexp.limits.at(2);
            maxy = varexp.limits.at(3);
            hist2d = TH2D("TEMP", title.c_str(), bins_x, minx, maxx, bins_y, miny, maxy);
            for (int i = 0; i < n; ++i) {
                if (data_x[i] >= minx && data_x[i] <= maxx && data_y[i] >= miny && data_y[i] <= maxy) {
                    hist2d.Fill(data_x[i], data_y[i]);
                }
            }
            hist2d.Draw("goff");
        }
        else {
            hist2d = TH2D("TEMP", title.c_str(), bins_x, minx, maxx, bins_y, miny, maxy);
            hist2d.FillN(n, data_x, data_y, nullptr);
            hist2d.Draw("goff");
        }

        if (minx == maxx) {
            // Handle single value histograms
            minx = minx - 1;
            maxx = minx + 1;
        }
        if (miny == maxy) {
            // Handle single value histograms
            miny = miny - 1;
            maxy = miny + 1;
        }
        AxisTicks xaxis(minx, maxx);
        AxisTicks yaxis(miny, maxy, 5, logscale);

        plotYAxis(yaxis, true);
        plotXAxis(xaxis, true);
        plotASCIIHistogram2D(&hist2d, bins_y, bins_x);
        plotCanvasAnnotations(&hist2d);
    }
    else {
        console.setError("Branch not found");
    }

    refresh();
}

void FileBrowser::plotASCIIHistogram(TH1D* hist, int binsy, int binsx) const {
    double max_height = hist->GetAt(hist->GetMaximumBin())*top_hist_clear;
    double pixel_y = max_height / binsy;

    if (logscale) {
        max_height = log(max_height) + 1.0f;  //+1 order of base magnitude for plotting
        pixel_y = max_height / binsy;
    }
    // Draw ASCII art
    wattron(main_window, COLOR_PAIR(col_whiteblue));
    for (int x = 0; x < binsx / 2; x++) {
        auto cl = hist->GetBinContent(2 * x);
        auto cr = hist->GetBinContent(2 * x + 1);
        if (logscale) {
            cl = log(cl);
            cr = log(cr);
        }

        if (blockmode == 4) {
            wattron(main_window, A_BOLD);
            for (int y = 0; y < binsy / 4; ++y) {
                // Check if ascii character is filled
                std::uint8_t probe = BLOCKS_code_4x2::BC_VOID;
                probe |= ((4*y + 0) * pixel_y < cl) << 7;
                probe |= ((4*y + 0) * pixel_y < cr) << 6;
                probe |= ((4*y + 1) * pixel_y < cl) << 5;
                probe |= ((4*y + 1) * pixel_y < cr) << 4;
                probe |= ((4*y + 2) * pixel_y < cl) << 3;
                probe |= ((4*y + 2) * pixel_y < cr) << 2;
                probe |= ((4*y + 3) * pixel_y < cl) << 1;
                probe |= ((4*y + 3) * pixel_y < cr) << 0;
                if (probe == BLOCKS_code_4x2::BC_VOID) {
                    // fill rest with blanks, prevents overdraw...
                    for (int f = y; f < binsy / 4; ++f) {
                        mvwprintw(main_window, mainwin_y-2-f, 1+x, " ");
                    }
                    break;
                }
                try {
                    auto print = ascii_4x2[ascii_map_4x2.at(static_cast<BLOCKS_code_4x2>(probe))];
                    mvwprintw(main_window, mainwin_y-2-y, 1+x, "%s", print);
                }
                catch(...) {
                    mvwprintw(main_window, mainwin_y-2-y, 1+x, "%i", probe);
                    mvwprintw(main_window, 0, 0, "%i", probe);
                    clrtoeol();
                }
            }
            wattroff(main_window, A_BOLD);
        }
        else if (blockmode == 3) {
            for (int y = 0; y < binsy / 3; ++y) {
                // Check if ascii character is filled
                std::uint8_t probe = BLOCKS_code_3x2::EC_VOID;
                probe |= ((3*y + 0) * pixel_y < cl) << 5;
                probe |= ((3*y + 0) * pixel_y < cr) << 4;
                probe |= ((3*y + 1) * pixel_y < cl) << 3;
                probe |= ((3*y + 1) * pixel_y < cr) << 2;
                probe |= ((3*y + 2) * pixel_y < cl) << 1;
                probe |= ((3*y + 2) * pixel_y < cr) << 0;
                if (probe == BLOCKS_code_3x2::EC_VOID) {
                    // fill rest with blanks, prevents overdraw...
                    for (int f = y; f < binsy / 3; ++f) {
                        mvwprintw(main_window, mainwin_y-2-f, 1+x, " ");
                    }
                    break;
                }
                try {
                    auto print = ascii_3x2[ascii_map_3x2.at(static_cast<BLOCKS_code_3x2>(probe))];
                    mvwprintw(main_window, mainwin_y-2-y, 1+x, "%s", print);
                }
                catch(...) {
                    mvwprintw(main_window, mainwin_y-2-y, 1+x, "%i", probe);
                    clrtoeol();
                }
            }
        }
        else if (blockmode == 2) {
            for (int y = 0; y < binsy / 2; ++y) {
                // Check if ascii character is filled
                std::uint8_t probe = BLOCKS_code_2x2::C_VOID;
                probe |= ((2*y + 0) * pixel_y < cl) << 3;
                probe |= ((2*y + 0) * pixel_y < cr) << 2;
                probe |= ((2*y + 1) * pixel_y < cl) << 1;
                probe |= ((2*y + 1) * pixel_y < cr);
                if (probe == BLOCKS_code_2x2::C_VOID) {
                    // fill rest with blanks, prevents overdraw...
                    for (int f = y; f < binsy / 2; ++f) {
                        mvwprintw(main_window,mainwin_y-2-f, 1+x, " ");
                    }
                    break;
                }
                try {
                    auto print = ascii_2x2[ascii_map_2x2.at(static_cast<BLOCKS_code_2x2>(probe))];
                    mvwprintw(main_window, mainwin_y-2-y, 1+x, "%s", print);
                }
                catch(...) {
                    mvwprintw(main_window, mainwin_y-2-y, 1+x, "%i", probe);
                }
            }
        }

    }
    wattroff(main_window, COLOR_PAIR(1));
    wrefresh(main_window);
}

void FileBrowser::plotASCIIHistogram2D(TH2D* hist, int binsy, int binsx) {
    double max_height = hist->GetAt(hist->GetMaximumBin());

    wclear(main_window);
    wrefresh(main_window);

    // Draw ASCII art
    for (int x = 0; x < binsx + 1; x++) {
        for (int y = 0; y < binsy + 1; y++) {
            auto Z = hist->GetBinContent(x, y);
            if (Z == 0) continue;
            auto pixel_color = std::lerp(col_grayscale_start, col_grayscale_end + 1, Z / max_height);
            pixel_color = std::clamp<int>(pixel_color, col_grayscale_start, col_grayscale_end - 1);
            wattron(main_window, COLOR_PAIR(pixel_color));
            mvwprintw(main_window, binsy + 1 - y, x, "█");
            wattroff(main_window, COLOR_PAIR(pixel_color));
        }
    }
    wrefresh(main_window);
}

void FileBrowser::plotCanvasAnnotations(TH1* hist) {
    // Plot Title
    // box(main_window, 0, 0);
    wattron(main_window, A_ITALIC | A_BOLD);
    if (logscale) {
        mvwprintw(main_window, 0, 4, "┤ %s (log-y) ├", hist->GetTitle());
    }
    else {
        mvwprintw(main_window, 0, 4, "┤ %s ├", hist->GetTitle());
    }
    wattroff(main_window, A_ITALIC | A_BOLD);

    // Plot stats
    int line = 1;
    if (showstats) {
        mvwprintw(main_window, line++, mainwin_x - 30, "Entries: %f",   hist->GetEntries());
        mvwprintw(main_window, line++, mainwin_x - 30, "Mean:    %.5f", hist->GetMean());
        mvwprintw(main_window, line++, mainwin_x - 30, "Std:     %.5f", hist->GetStdDev());
        mvwprintw(main_window, line++, mainwin_x - 30, "Bins:    %i",   hist->GetNbinsX());
    }

    if (hist->GetEntries() == 0) {
        wattron(main_window, A_BOLD);
        mvwprintw(main_window, mainwin_y / 2, mainwin_x / 2 - 3, "Empty");
        wattroff(main_window, A_BOLD);
    }
    wrefresh(main_window);
}

void FileBrowser::plotCanvasAnnotations(TH2* hist) {
    // Plot Title
    box(main_window, 0, 0);
    wattron(main_window, A_ITALIC | A_BOLD);
    if (logscale) {
        mvwprintw(main_window, 0, 4, "┤ %s (log-y) ├", hist->GetTitle());
    }
    else {
        mvwprintw(main_window, 0, 4, "┤ %s ├", hist->GetTitle());
    }
    wattroff(main_window, A_ITALIC | A_BOLD);

    // Plot stats
    int line = 1;
    if (showstats) {
        mvwprintw(main_window, line++, mainwin_x - 30, "Entries: %f",   hist->GetEntries());
        mvwprintw(main_window, line++, mainwin_x - 30, "Mean:    %.5f", hist->GetMean());
        mvwprintw(main_window, line++, mainwin_x - 30, "Std:     %.5f", hist->GetStdDev());
        mvwprintw(main_window, line++, mainwin_x - 30, "x Bins:  %i",   hist->GetNbinsX());
        mvwprintw(main_window, line++, mainwin_x - 30, "y Bins:  %i",   hist->GetNbinsY());
        mvwprintw(main_window, line++, mainwin_x - 30, "corr:    %f",   hist->GetCorrelationFactor());
    }

    if (hist->GetEntries() == 0) {
        wattron(main_window, A_BOLD);
        mvwprintw(main_window, mainwin_y / 2, mainwin_x / 2 - 3, "Empty");
        wattroff(main_window, A_BOLD);
    }
    wrefresh(main_window);
}

void FileBrowser::plotXAxis(AxisTicks& ticks, bool force_range) {
    int sizex = getmaxx(main_window);
    int sizey = getmaxy(main_window);
    int winx = getbegx(main_window);
    int winy = getbegy(main_window);

    move(sizey + 1, 0);
    clrtobot();

    // Write numbers below axis at tick positions
    auto nchars = sizex - 2;
    ticks.setAxisPixels(nchars);

    std::vector<int> written_positions;
    for (int i = 0; i < ticks.nticks; ++i) {
        auto tick = ticks.getTick(i, !force_range);
        written_positions.push_back(tick.char_position);
        mvprintw(winy + sizey - 1, winx + 1 + tick.char_position, "┼");

        if (ticks.E != 0) { attron(COLOR_PAIR(col_yellow)); }
        attron(A_ITALIC | A_BOLD);
        mvprintw(winy + sizey, winx + 1 + tick.char_position - tick.tickstr.size() / 2, "%s", tick.tickstr.c_str());
        attroff(A_ITALIC | A_BOLD);
        if (ticks.E != 0) { attroff(COLOR_PAIR(col_yellow)); }
    }
    for (int i = 0; i < nchars; ++i) {
        if (std::find(written_positions.begin(), written_positions.end(), i) == written_positions.end()) {
            mvprintw(winy + sizey - 1, winx + 1 + i, "─");
        }
    }

    // Print exponent if needed
    if (ticks.E != 0) {
        attron(COLOR_PAIR(col_yellow));
 #if USE_UNICODE==1
        std::string magnitude = fmtstring("x10{}", make_superscript(ticks.E));
 #else
        std::string magnitude = fmtstring("x10^{}", make_superscript(ticks.E));
 #endif
        mvprintw(winy + sizey-1, winx + sizex - magnitude.size() - 2, " %s ", magnitude.c_str());
        attroff(COLOR_PAIR(col_yellow));
    }
}

void FileBrowser::plotYAxis(AxisTicks& ticks, bool force_range) {
    int reserve_xwidth = ticks.maxLabelWidth() + 1;
    if (reserve_xwidth > yaxis_spacing) {
        makeSpaceForYaxis(reserve_xwidth);
    }
    int winx = getbegx(main_window);
    int winy = getbegy(main_window);
    int sizey = getmaxy(main_window);

    // Prevent overdraw
    for (int i = 0; i < mainwin_y; ++i) {
        for (int j = 0; j < yaxis_spacing; ++j) {
            mvaddch(winy + i, winx + j - yaxis_spacing, ' ');
        }
    }

    // Write numbers below axis at tick positions
    auto nchars = sizey - 2;
    ticks.setAxisPixels(nchars);

    std::vector<int> written_positions;
    for (int i = 0; i < ticks.nticks; ++i) {
        auto tick = ticks.getTick(i, !force_range);
        written_positions.push_back(tick.char_position);
        mvprintw(winy + sizey - 1 - tick.char_position, winx, "┼");
 
        if (ticks.E != 0) { attron(COLOR_PAIR(col_yellow)); }
        attron(A_ITALIC | A_BOLD);
        mvprintw(winy + sizey - 1 - tick.char_position, winx - tick.tickstr.size(), "%s", tick.tickstr.c_str());
        attroff(A_ITALIC | A_BOLD);
        if (ticks.E != 0) { attroff(COLOR_PAIR(col_yellow)); }
    }
    // Print exponent if needed
    if (ticks.E != 0) {
        attron(COLOR_PAIR(col_yellow));
 #if USE_UNICODE==1
        std::string magnitude = fmtstring("x10{}", make_superscript(ticks.E));
 #else
        std::string magnitude = fmtstring("x10^{}", make_superscript(ticks.E));
 #endif
        mvprintw(winy - 1, winx, " %s ", magnitude.c_str());
        attroff(COLOR_PAIR(col_yellow));
    }
    else {
        move(winy - 1, winx);
        clrtoeol();
    }
}

void FileBrowser::refreshCMDWindow() {
    int posx = getbegx(cmd_window) + 1;
    int posy = getbegy(cmd_window) + 1;
    console.redraw(posy, posx);
    wattron(cmd_window, COLOR_PAIR(col_blue));
    box(cmd_window, 0, 0);
    wattroff(cmd_window, COLOR_PAIR(col_blue));
    wrefresh(cmd_window);
}

bool FileBrowser::isClickInWindow(WINDOW*& win, int y, int x) {
    int posy, posx;
    int sizey, sizex;
    getmaxyx(win, sizey, sizex);
    getbegyx(win, posy, posx);
    return x > posx && x < posx + sizex - 1 && y > posy && y < posy + sizey - 1;
}

void FileBrowser::handleMouseClick(int y, int x) {
    // Clicked inside window?
    if (isClickInWindow(dir_window, y, x)) {
        const int posy = getbegy(dir_window);
        selected_pos = y - posy - 1;
        handleMenuSelect();
    }
    else if (colorWindow.show && isClickInWindow(colorWindow.color_window, y, x)) {
        colorWindow.selectFromMouseClick(y, x);
    }
    else if (isClickInWindow(cmd_window, y, x)) {
        console.entering_draw_command = true;
        int posx = x - getbegx(cmd_window) - 1;
        console.cursorMove(posx);
        refreshCMDWindow();
    }
}

void FileBrowser::handleInputEvent(MEVENT& mouse_event, int key) {
    handleResize();
    if (key == KEY_MOUSE) {
        // Mouse clicks take precedence over console input mode. Always handled
        if (getmouse(&mouse_event) == OK) {
            if (mouse_event.bstate == BUTTON1_PRESSED) {
                handleMouseClick(mouse_event.y, mouse_event.x);
            }
            else if (mouse_event.bstate == BUTTON4_PRESSED) {
                selectionUp();
            }
            else if (mouse_event.bstate == BUTTON5_PRESSED) {
                selectionDown();
            }
        }
        refreshCMDWindow();
        drawEssentials();
        return;
    }

    if (searchMode.isActive) {
        if (SearchMode::isBranchChar(key)) {
            searchMode.input += static_cast<char>(key);
            updateSearchResults();
            return;
        }
        else if (key == KEY_BACKSPACE || key == KEY_DL) {
            if (!searchMode.input.empty()) {
                searchMode.input.pop_back();
                updateSearchResults();
                return;
            }
        }
    }

    if (colorWindow.show) {
        colorWindow.render();
    }

    if (console.entering_draw_command && !searchMode.isActive) {
        if (key == KEY_ENTER || key == 10) {
            if (console.parse()) {
                plotHistogram();
            }
            console.entering_draw_command = false;
        }
        else {
            console.handleInput(key);
        }
        refreshCMDWindow();
        return;
    }

    bool invalid_key = false;

    switch (key) {
        case KEY_DOWN:
            selectionDown();
            break;
        case KEY_UP:
            selectionUp();
            break;
        case KEY_F(1):
            menu_width = std::min<int>(menu_width + 3, getmaxx(stdscr) - 10);
            handleResize(true);
            break;
        case KEY_F(2):
            menu_width = std::max<int>(menu_width - 3, 10);
            handleResize(true);
            break;
        case 'q':
            is_running = false;
            break;
        case 'g':
            goTop();
            break;
        case 'G':
            goBottom();
            break;
        case '/':
            searchMode.isActive = !searchMode.isActive;
            if (searchMode.isActive) {
                searchMode.input.clear();
            }
            break;
        case 's':
            toggleStatsBox();
            plotHistogram();
            break;
        case 'l':
            toggleLogy();
            plotHistogram();
            break;
        case 't':
            toggleBlockMode();
            plotHistogram();
            break;
        case 'd':
            console.entering_draw_command = true;
            break;
        case 'C':
            colorWindow.show = !colorWindow.show;
            break;
        case KEY_ENTER: case 10: // ENTER only works with RightShift+Enter
            handleMenuSelect();
            break;
        case '?':
            helpWindow();
            skipDraw = true;
            break;
        default:
            if (console.validChar(key)) {
                nonsense.push_back(key);
            }
            invalid_key = true;
            break;
    }
    refreshCMDWindow();
    drawEssentials();

    if (!invalid_key) {
        nonsense.clear();
    }

    if (invalid_key && nonsense.size() > 5) {
        // User forgot to enter input mode?
        console.current_input = nonsense;
        nonsense.clear();
        console.entering_draw_command = true;
    }
    refresh();
}

void FileBrowser::updateSearchResults() {
    for (auto& [name, node] : root_file.displayList) {
        if (node->type == NodeType::TLEAF) {
            node -> showInSearch = string_contains(name, searchMode.input);
        }
        else {
            node->showInSearch = false; // Make sure this is always hidden
        }
    }
}

bool FileBrowser::SearchMode::isBranchChar(int key) {
    return key > 0 && key < 256 && (isalnum(key) || key == '_');
}

void FileBrowser::makeSpaceForYaxis(int s) {
    yaxis_spacing = std::max<int>(yaxis_spacing, s);
    handleResize(true);
}

void FileBrowser::handleResize(bool force) {
    if (resize_flag || force) {
        resize_flag = false;

        endwin();  // required
        refresh(); // required
        clear();   // required

        resize_term(LINES, COLS);

        initAllWindows();
        
        box(main_window, 0, 0);
        box(dir_window, 0, 0);
        printDirectories();
        refreshCMDWindow();
        wrefresh(main_window);
        wrefresh(dir_window);
        wrefresh(cmd_window);
        refresh();

        getmaxyx(main_window, mainwin_y, mainwin_x);
    }
}

void FileBrowser::createWindow(WINDOW*& win, int size_y, int size_x, int pos_y, int pos_x) {
    if (win != nullptr) {
        delwin(win);
    }
    win = newwin(size_y, size_x, pos_y, pos_x);
}

void FileBrowser::drawEssentials() {
    std::string help = "Preferences <P> Show help <?>";
    mvprintw(0, getmaxx(stdscr) - help.size(), "%s", help.c_str());
}

void FileBrowser::handleMenuSelect() {
    auto fetch = root_file.getEntry(selected_pos + menu_scroll_pos, searchMode.isActive);
    if (!fetch.has_value()) {
        return;
    }

    auto& [name, node] = fetch.value();
    if (node->type == NodeType::DIRECTORY || node->type == NodeType::TTREE) {
        node->toggleOpenOnClick();
    }
    else if (node->type == NodeType::TLEAF) {
        console.clearCommand();
        plotHistogram(root_file.m_trees.at(node->mother->index), 
                      root_file.m_leaves.at(node->index));
    }
}

void FileBrowser::helpWindow() {
    int margin = 6;
    WINDOW* help = newwin(getmaxy(stdscr) - 2*margin, getmaxx(stdscr) - 2* margin, margin, margin);

    int line = 0;
    auto helpline = [&line, &help] (std::string text) {
        mvwprintw(help, ++line, 2, "%s", text.c_str());
    };

    attron(A_UNDERLINE);
    helpline(" HELP ");
    attroff(A_UNDERLINE);
    line++;
    helpline("Show help ............ <?>");
    helpline("Toggle search mode ... </>");
    helpline("Move through menu .... <Arrow keys/Mouse Wheel>");
    helpline("Open Directory/TTree . <ENTER>");
    helpline("Toggle stats box ..... <s>");
    helpline("Enter draw command ... <d>");
    helpline("Toggle log y ......... <l>");
    helpline("Go to top ............ <g>");
    helpline("Go to bottom ......... <G>");
    helpline("Plot selected ........ <ENTER/LMB>");
    helpline("Cycle graphics mode .. <t>");
    helpline("Resize object menu ... <F1/F2>");
    helpline("Quit ................. <q/Ctrl+C>");

    line = 0;
    attron(A_UNDERLINE);
    mvwprintw(help, ++line, 53, "Terminal capabilities (health check)");
    attroff(A_UNDERLINE);
    line++;
    mvwprintw(help, ++line, 53, "Color support: ........... %s (required)", has_colors() ? "YES" : "NO");
    mvwprintw(help, ++line, 53, "Extended color support: .. %s (required)", can_change_color() ? "YES" : "NO");
    mvwprintw(help, ++line, 53, "Compiled with unicode: ... %s", USE_UNICODE == 1 ? "YES" : "NO");
    mvwprintw(help, ++line, 53, "Terminal colors: ......... %i (needs 256)", tigetnum("colors"));
    mvwprintw(help, ++line, 53, "Graphics characters 2x2 .. ▖,▗,▄,▌,▐,▙,▟,█ (required)");
    mvwprintw(help, ++line, 53, "Graphics characters 3x2 .. 🬏,🬞,🬭,🬱,🬵,🬹,🬓,🬦,▌,▐,🬲,🬷,🬺,🬻,█");
    mvwprintw(help, ++line, 53, "Graphics characters 4x2 .. ⡀,⢀,⡄,⢠,⡆,⢰,⡇,⢸,⣀,⣤,⣶,⣿,⣄,⣠,⣆,⣰,⣇,⣸,⣦,⣴,⣧,⣼,⣷,⣾");
    mvwprintw(help, ++line, 53, "Max color pairs........... %i", COLOR_PAIRS);
    mvwprintw(help, ++line, 53, "LC_ALL ................... %s", setlocale(LC_ALL, nullptr));
    mvwprintw(help, ++line, 53, "LC_CTYPE ................. %s", setlocale(LC_CTYPE, nullptr));
    mvwprintw(help, ++line, 53, "LC_MESSAGES .............. %s", setlocale(LC_MESSAGES, nullptr));
    mvwprintw(help, ++line, 53, "NUCRSES .................. %i.%i.%i", NCURSES_VERSION_MAJOR, NCURSES_VERSION_MINOR, NCURSES_VERSION_PATCH);

    ++line;
    mvwprintw(help, ++line, 53, "!Solid color gradients should be displayed below!");
    ++line;

    line++;
    mvwprintw(help, line++, 53, "26 grayscale steps");
    line++;
    for (int j = 0, i = col_grayscale_start; i < col_grayscale_end; ++i, ++j) {
        mvwprintw(help, line - 1, 53 + j, "v");
        wattron(help, COLOR_PAIR(i));
        mvwprintw(help, line, 53 + j, "█");
        mvwprintw(help, line + 1, 53 + j, "^");
        wattroff(help, COLOR_PAIR(i));
    }

    attron(A_ITALIC);
    mvprintw(getbegy(help) + getmaxy(help) - 2, getbegy(help) + 1, "Repository: github.com/VukanJ/tbrowser");
    attroff(A_ITALIC);

    wattron(help, COLOR_PAIR(col_yellow));
    box(help, 0, 0);
    wattroff(help, COLOR_PAIR(col_yellow));
    wbkgd(help, COLOR_PAIR(col_win_bkg));
    wrefresh(help);
    delwin(help);
}

void FileBrowser::ColorPickerWindow::render() const {
    box(color_window, 0, 0);
    if (color_window == nullptr) throw;

    wattron(color_window, COLOR_PAIR(selected) | A_BLINK | A_REVERSE);
    mvwprintw(color_window, 0, 2, "Pick a color");
    wattroff(color_window, COLOR_PAIR(selected) | A_BLINK | A_REVERSE);

    wmove(color_window, 1, 1);
    int line = 1;
    wattron(color_window, A_REVERSE);
    for (int c = col_start_palette - 1; c < col_grayscale_end; ) {
        for (int j = 0; j < 18; ++j, c++) {
            if (c >= col_grayscale_end || c < col_start_palette) {
                wattroff(color_window, A_REVERSE);
                wprintw(color_window, "XX");
                wattron(color_window, A_REVERSE);
            }
            else {
                wattron(color_window, COLOR_PAIR(c));
                wprintw(color_window, "  ");
                wattroff(color_window, COLOR_PAIR(c));
            }
        }
        wmove(color_window, ++line, 1);
    }
    wattroff(color_window, A_REVERSE);
    
    wrefresh(color_window);
}

void FileBrowser::ColorPickerWindow::init() {
    createWindow(this->color_window, 16, 38, 10, 50);
}

FileBrowser::ColorPickerWindow::~ColorPickerWindow() {
    if (color_window != nullptr) {
        delwin(color_window);
    }
}

TermColor FileBrowser::ColorPickerWindow::selectFromMouseClick(int y, int x) {
    const int begy  = getbegy(color_window);
    const int begx  = getbegx(color_window);
    y -= begy;
    x -= begx;
    auto sel = static_cast<TermColor>(col_start_palette + (y * 18) + x / 2);
    selected = sel;
    return sel;
}
