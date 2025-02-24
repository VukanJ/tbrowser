#include "Browser.h"

#include <cctype>
#include <cmath>
#include <csignal>
#include <format>
#include <algorithm>
#include <string>
#include <unistd.h>
#include <unordered_map>

#include <ncurses.h>

#include "RtypesCore.h"
#include "TTree.h"
#include "TTreeFormula.h"

#include "AxisTicks.h"
#include "RootFile.h"
#include "definitions.h"

constexpr std::array<const char[4], 8> ascii { "▖", "▗", "▄", "▌", "▐", "▙", "▟", "█" };

auto make_superscript(int n) {
#if USE_UNICODE
    std::string sup;
    constexpr std::array<const char[4], 10> super { "⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹" };
    for (auto c : std::to_string(n)) {
        sup += c == '-' ? ASCII_SUP_MINUS : super[c - '0'];
    }
    return sup;
#else
    return std::to_string(n);
#endif
}

volatile bool resize_flag = false;

FileBrowser::FileBrowser() {
    initNcurses();
    int sizex = getmaxx(stdscr);
    int sizey = getmaxy(stdscr);
    createWindow(main_window, sizey - bottom_height, sizex - menu_width, 1, menu_width);
    createWindow(dir_window, sizey - bottom_height, menu_width, 1, 0);
    createWindow(cmd_window, 3, sizex - 5 - menu_width, sizey - bottom_height + 3, 20 + 5);

    refresh();
    box(dir_window, 0, 0);
    box(main_window, 0, 0);
    wrefresh(main_window);

    refreshCMDWindow();

    getmaxyx(stdscr, terminal_size_y, terminal_size_x);
    drawEssentials();
}

FileBrowser::~FileBrowser() {
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
    signal(SIGWINCH, [](int) { resize_flag = true; });

    mouseinterval(0);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    start_color();
    init_pair(blue, COLOR_BLUE, COLOR_BLACK);
    init_pair(green, COLOR_GREEN, COLOR_BLACK);
    init_pair(red, COLOR_RED, COLOR_BLACK);
    init_pair(white, COLOR_WHITE, COLOR_BLACK);
    init_pair(yellow, COLOR_YELLOW, COLOR_BLACK);
    init_pair(white_on_blue, COLOR_WHITE, COLOR_BLUE);
    init_pair(whiteblue, 33, COLOR_BLACK);

    // initialize grayscale
    int grayscale = grayscale_start;
    init_pair(grayscale++, COLOR_BLACK, COLOR_BLACK);
    for (int c = 232; c <= 255; ++c) {
        init_pair(grayscale++, c, COLOR_BLACK);
    }
    init_pair(grayscale++, COLOR_WHITE, COLOR_BLACK);
}

void FileBrowser::loadFile(std::string filename) {
    mvprintw(1, 1, "Reading...");
    refresh();

    root_file.load(filename);
}

void FileBrowser::printDirectories() {
    if (skipDraw) { skipDraw = false; return; }
    int x = getbegx(dir_window) + 1;
    int y = getbegy(dir_window) + 1;
    int maxlines = getmaxy(dir_window) - 2;

    int entry = -menu_scroll_pos;
    auto print_entry = [this, &entry, y, x](std::string name, RootFile::Node* node){
        std::string entry_label;
        name = std::string(node->nesting * 2, ' ') + name;
        color col = white;
        int attr = A_NORMAL;
        switch (node->type) {
            case NodeType::DIRECTORY:
                if (node->openState & RootFile::Node::DIR_OPEN) 
                    { entry_label = std::format("{} {}", SYMB_FOLDER_OPEN, name); }
                else
                    { entry_label = std::format("{} {}", SYMB_FOLDER_CLOSED, name); }
                col = yellow;
                break;
            case NodeType::TLEAF:   entry_label = std::format("{} {}", SYMB_TLEAF, name); attr = A_ITALIC; break;
            case NodeType::TTREE:   entry_label = std::format("{} {}", SYMB_TTREE, name); col = green; attr = A_BOLD; break;
            case NodeType::HIST:    entry_label = std::format("{} {}", SYMB_THIST, name); col = blue; attr = A_ITALIC; break;
            case NodeType::UNKNOWN: entry_label = std::format("{} {}", SYMB_TUNKNOWN, name); col = red; break;
        }
        attron(COLOR_PAIR(col));
        attron(attr);
        if (entry == selected_pos) {
            attron(A_REVERSE);
            attron(A_ITALIC);
        }
        mvprintw(y + entry, x, "%.20s", entry_label.c_str());
        if (entry == selected_pos) {
            attroff(A_REVERSE);
            attroff(A_ITALIC);
        }
        attroff(attr);
        attroff(COLOR_PAIR(col));
        entry++;
    };

    for (const auto& [name, node] : root_file.displayList) {
        if (entry < 0) entry++;
        else if (entry < maxlines) {
            if (node->openState & RootFile::Node::LISTED) {
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
    wrefresh(dir_window);
}

void FileBrowser::selectionDown() {
    if (selected_pos == getmaxy(dir_window) - 3) {
        menu_scroll_pos++;
    }
    else {
        selected_pos = std::min<int>(std::min<int>(root_file.menuLength(), getmaxy(dir_window) - 3), selected_pos + 1); 
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
    menu_scroll_pos = root_file.menuLength() - getmaxy(dir_window) + 2;
}

void FileBrowser::toggleKeyBindings() {
    showkeys = !showkeys; 
}

void FileBrowser::toggleStatsBox() {
    showstats = !showstats; 
} 

void FileBrowser::toggleLogy() {
    logscale = !logscale; 
} 

void FileBrowser::plotHistogram() {
    // Get currently active TTree
    if (console.hasCommand()) {
        plotHistogram(console.current_args);
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

void FileBrowser::plotHistogram(TTree* tree, TLeaf* leaf) {
    // Get window position and size
    int winx = getbegx(main_window);
    int winy = getbegy(main_window);
    getmaxyx(main_window, mainwin_y, mainwin_x);
    box(main_window, 0, 0);

    const char* leafname = leaf->GetName();

    // Get bounds
    auto bins_x = 2 * mainwin_x - 4;  // Border has width of 4 bins
    auto bins_y = 2 * mainwin_y - 4;
    auto min = tree->GetMinimum(leafname);
    auto max = tree->GetMaximum(leafname);
    if (min == max) {
        // Handle single value histograms
        min = min - 1;
        max = min + 1;
    }

    const AxisTicks xaxis(min, max);
    min = xaxis.min_adjusted();
    max = xaxis.max_adjusted();

    TH1D hist("H", leafname, bins_x, min, max);

    tree->Project("H", leafname);
    
    plotASCIIHistogram(winy, winx, &hist, bins_y, bins_x);
    plotCanvasAnnotations(&hist, winy, winx);
    plotAxes(xaxis, winy, winx, mainwin_y, mainwin_x);

    refresh();
}

void FileBrowser::plotHistogram(const Console::DrawArgs& args) {
    // Get window position and size
    int winx = getbegx(main_window);
    int winy = getbegy(main_window);
    getmaxyx(main_window, mainwin_y, mainwin_x);
    box(main_window, 0, 0);

    auto& [varexp, selection, option, nentries, firstentry] = args;

    // Get selected tree
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
        console.setError("No TTree opened");
        return; // Give up for now
    }

    mvprintw(4, 30, "A1 %s", varexp.c_str());
    mvprintw(5, 30, "A2 %s", selection.c_str());
    mvprintw(6, 30, "A3 %s", option);
    mvprintw(7, 30, "A4 %lld", nentries);
    mvprintw(8, 30, "A5 %lld", firstentry);

    // Get bounds
    auto bins_x = 2 * mainwin_x - 4;  // Border has width of 4 bins
    auto bins_y = 2 * mainwin_y - 4;

    ttree->SetEstimate(ttree->GetEntries());
    Long64_t entriesDrawn = -1;
    try {
        entriesDrawn = ttree->Draw(varexp.c_str(), selection.c_str(), option, nentries, firstentry);
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

    if (TLeaf* lenLeaf = vector_branches(varexp.c_str()); lenLeaf != nullptr && false) {
        // AFAIK, an efficient loop is required now
        // SKIP!
        TTreeFormula formula("FORM", varexp.c_str(), ttree);
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
            title = varexp;
        }
        else {
            title = std::format("{} ({})", varexp, selection);
        }

        TH1D newhist("TEMP", title.c_str(), bins_x, min, max);
        newhist.FillN(n, data, nullptr);
        newhist.Draw("goff");

        mvprintw(10, 30, "HIST");
        mvprintw(11, 30, "name %s", newhist.GetName());
        mvprintw(12, 30, "title %s", newhist.GetTitle());
        mvprintw(13, 30, "max %f", max);
        mvprintw(14, 30, "min %f", min);
        mvprintw(15, 30, "binsx %i", newhist.GetNbinsX());
        mvprintw(16, 30, "processed %lld", ttree->GetSelectedRows());
        mvprintw(17, 30, "n %lld", n);
        mvprintw(18, 30, "Entries %lld", ttree->GetEntries());
        mvprintw(19, 30, "first %f", data[0]);
        if (min == max) {
            // Handle single value histograms
            min = min - 1;
            max = min + 1;
        }
        const AxisTicks xaxis(min, max);
        min = xaxis.min_adjusted();
        max = xaxis.max_adjusted();

        plotASCIIHistogram(winy, winx, &newhist, bins_y, bins_x);
        plotCanvasAnnotations(&newhist, winy, winx);
        plotAxes(xaxis, winy, winx, mainwin_y, mainwin_x);
    }
    else {
        console.setError("Branch not found");
    }

    refresh();
}

void FileBrowser::plotASCIIHistogram(int winy, int winx, TH1D* hist, int binsy, int binsx) {
    double max_height = hist->GetAt(hist->GetMaximumBin())*1.1;
    double pixel_y = max_height / binsy;

    if (logscale) {
        max_height = log(max_height) + 1.0f;  //+1 order of base magnitude for plotting
        pixel_y = max_height / binsy;
    }
    // Draw ASCII art
    attron(COLOR_PAIR(whiteblue));
    for (int x = 0; x < binsx / 2; x++) {
        auto cl = hist->GetBinContent(2 * x);
        auto cr = hist->GetBinContent(2 * x + 1);
        if (logscale) {
            cl = log(cl);
            cr = log(cr);
        }
        for (int y = 0; y < binsy / 2; ++y) {
            // Check if ascii character is filled
            std::uint8_t probe = ASCII_code::C_VOID;
            probe |= ((2*y + 0) * pixel_y < cl) << 3;
            probe |= ((2*y + 0) * pixel_y < cr) << 2;
            probe |= ((2*y + 1) * pixel_y < cl) << 1;
            probe |= ((2*y + 1) * pixel_y < cr);
            if (probe == ASCII_code::C_VOID) {
                // fill rest with blanks, prevents overdraw...
                for (int f = y; f < binsy / 2; ++f) {
                    mvprintw(winy+mainwin_y-2-f, winx+1+x, " ");
                }
                break;
            }
            try {
                auto print = ascii[ascii_map.at(static_cast<ASCII_code>(probe))];
                mvprintw(winy+mainwin_y-2-y, winx+1+x, "%s", print);
            }
            catch(...) {
                mvprintw(winy+mainwin_y-2-y, winx+1+x, "%i", probe);
            }

        }
    }
    attroff(COLOR_PAIR(1));
}

void FileBrowser::plotCanvasAnnotations(TH1* hist, int winy, int winx) {
    // Plot Title
    box(main_window, 0, 0);
    wrefresh(main_window);
    attron(A_ITALIC);
    attron(A_UNDERLINE);
    if (logscale) {
        mvprintw(1, winx+2, "┤ log(%s) ├", hist->GetTitle());
    }
    else {
        mvprintw(1, winx+2, "┤ %s ├", hist->GetTitle());
    }
    attroff(A_UNDERLINE);
    attroff(A_ITALIC);

    // Plot stats
    int line = 1;
    if (showstats) {
        mvprintw(winy + line++, winx + mainwin_x - 30, "Entries: %f",   hist->GetEntries());
        mvprintw(winy + line++, winx + mainwin_x - 30, "Mean:    %.5f", hist->GetMean());
        mvprintw(winy + line++, winx + mainwin_x - 30, "Std:     %.5f", hist->GetStdDev());
        mvprintw(winy + line++, winx + mainwin_x - 30, "Bins:    %i",   hist->GetNbinsX());
    }

    if (hist->GetEntries() == 0) {
        attron(A_BOLD);
        mvprintw(winy + mainwin_y / 2, winx + mainwin_x / 2 - 3, "Empty");
        attroff(A_BOLD);
    }
}

void FileBrowser::plotAxes(const AxisTicks& ticks, int winy, int winx, int sizey, int sizex) 
{
    // Clear line below plot
    move(winy + sizey, 0);
    clrtoeol();

    double xmin = ticks.min_adjusted();
    double xmax = ticks.max_adjusted();

    auto nBinsX = ticks.nticks;
    double rangeX = xmax - xmin;

    // Write numbers below axis at tick positions
    auto nchars = sizex - 1;
    std::vector<char> xaxis_chars(nchars, '-');
    if (ticks.E != 0) { attron(COLOR_PAIR(yellow)); }
    for (int i = 0; i < nBinsX; ++i) {
        double tickPos = ticks.values_d[i] * std::pow(10.0, ticks.E);
        int charpos = (tickPos - xmin) / rangeX * nchars;
        if (charpos < 0) continue;
        xaxis_chars[std::min<int>(charpos, nchars - 1)] = '+';

        attron(A_ITALIC);
        attron(A_BOLD);
        if (ticks.integer) {
            // write centered numbers below
            long tick = ticks.values_i[i];
            int ticklen = ticks.values_str[i].size();
            mvprintw(winy + sizey, winx + 1 + charpos - ticklen / 2, "%ld", tick);
        }
        else {
            // write centered numbers below
            auto num = ticks.values_str[i];
            int ticklen = num.size();
            mvprintw(winy + sizey, winx + 1 + charpos - ticklen / 2, "%s", num.c_str());
        }
        attroff(A_BOLD);
        attroff(A_ITALIC);
    }
    if (ticks.E != 0) { attroff(COLOR_PAIR(yellow)); }
    
    // Draw the tick lines
    for (int i = 0; char c : xaxis_chars) {
        mvprintw(winy + sizey - 1, winx + 1 + i++, c == '+' ? "┼" : "─"); 
    }
    mvprintw(winy + sizey - 1, winx + nchars, "┘");

    // Print exponent if needed
    if (ticks.E != 0) {
        attron(COLOR_PAIR(yellow));
#if USE_UNICODE
        std::string magnitude = std::format("x10{}", make_superscript(ticks.E));
#else
        std::string magnitude = std::format("x10^{}", make_superscript(ticks.E));
#endif
        mvprintw(winy+sizey-1, winx+sizex-magnitude.size()-2, " %s ", magnitude.c_str());
        attroff(COLOR_PAIR(yellow));
    }
}

void FileBrowser::refreshCMDWindow() {
    int posx = getbegx(cmd_window) + 1;
    int posy = getbegy(cmd_window) + 1;
    console.redraw(posy, posx);
    wattron(cmd_window, COLOR_PAIR(blue));
    box(cmd_window, 0, 0);
    wattroff(cmd_window, COLOR_PAIR(blue));
    wrefresh(cmd_window);
}

bool FileBrowser::isClickInWindow(WINDOW*& win, int y, int x) const {
    int posy, posx;
    int sizey, sizex;
    getmaxyx(win, sizey, sizex);
    getbegyx(win, posy, posx);
    if (x > posx && x < posx + sizex - 1 && y > posy && y < posy + sizey - 1) {
        return true;
    }

    return false;
}

void FileBrowser::handleMouseClick(int y, int x) {
    // Clicked inside window?
    if (isClickInWindow(dir_window, y, x)) {
        int posy = getbegy(dir_window);
        selected_pos = y - posy - 1;
        handleMenuSelect();
    }
    else if (isClickInWindow(cmd_window, y, x)) {
        console.entering_draw_command = true;
        int posx = x - getbegx(cmd_window) - 1;
        console.cursorMove(posx);
        refreshCMDWindow();
    }
}

void FileBrowser::handleInputEvent(MEVENT& mouse_event, int key) {
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
        return;
    }

    if (console.entering_draw_command) {
        if (key == KEY_ENTER || key == 10) {
            if (console.parse()) {
                plotHistogram();
            }
            console.clearCommand();
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
            mvprintw(30, 30, "SEARCH");
            break;
        case 's':
            toggleStatsBox();
            plotHistogram();
            break;
        case 'l':
            toggleLogy();
            plotHistogram();
            break;
        case 'd':
            console.entering_draw_command = true;
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
}

void FileBrowser::handleResize() {
    resize_flag = false;
    int sizex = getmaxx(stdscr);
    int sizey = getmaxy(stdscr);
    getmaxyx(stdscr, sizey, sizex);
    // !!! May be false positive, check
    if (true || sizex != terminal_size_x || sizey != terminal_size_y) {
        // mvprintw(getmaxy(dir_window), 0, "RESIZE %i,%i", sizey, sizex);
        terminal_size_x = sizex;
        terminal_size_y = sizey;
        endwin();
        refresh();
        createWindow(main_window, terminal_size_y - bottom_height, terminal_size_x - menu_width, 0, menu_width);
        createWindow(dir_window, terminal_size_y - bottom_height, menu_width, 0, 0);
    }
    else {
        // mvprintw(getmaxy(dir_window), 0, " FAKERESIZE %i,%i", sizey, sizex);
    }
}

void FileBrowser::createWindow(WINDOW*& win, int size_y, int size_x, int pos_y, int pos_x) {
    if (win) {
        delwin(win);
    }
    win = newwin(size_y, size_x, pos_y, pos_x);
    refresh();
    box(win, 0, 0);
    wrefresh(win);
}

void FileBrowser::drawEssentials() {
    std::string help = "Show help <?>";
    mvprintw(0, getmaxx(stdscr) - help.size(), "%s", help.c_str());
}

void FileBrowser::handleMenuSelect() {
    auto fetch = root_file.getEntry(selected_pos + menu_scroll_pos);
    if (!fetch.has_value()) {
        return;
    }

    auto& [name, node] = fetch.value();
    if (node->type == NodeType::DIRECTORY || node->type == NodeType::TTREE) {
        node->toggleOpenOnClick();
    }
    else if (node->type == NodeType::TLEAF) {
        plotHistogram(root_file.m_trees.at(node->mother->index), 
                      root_file.m_leaves.at(node->index));
    }
}

void FileBrowser::helpWindow() {
    int margin = 6;
    WINDOW* help = newwin(terminal_size_y - 2*margin, terminal_size_x - 2* margin, margin, margin);

    int line = 0;
    auto helpline = [&line, &help] (std::string text) {
        mvwprintw(help, ++line, 2, "%s", text.c_str());
    };

    attron(A_UNDERLINE);
    helpline(" HELP ");
    attroff(A_UNDERLINE);
    line++;
    helpline("Show help ............ <?>");
    helpline("Search branch ........ </>");
    helpline("Move through menu .... <Arrow keys/Mouse Wheel>");
    helpline("Open Directory/TTree . <ENTER>");
    helpline("Toggle stats box ..... <s>");
    helpline("Enter draw command ... <d>");
    helpline("Toggle log y ......... <l>");
    helpline("Go to top ............ <g>");
    helpline("Go to bottom ......... <G>");
    helpline("Plot selected ........ <ENTER/LMB>");
    helpline("Quit ................. <q/Ctrl+C>");

    line = 0;
    attron(A_UNDERLINE);
    mvwprintw(help, ++line, 53, "Terminal capabilities");
    attroff(A_UNDERLINE);
    line++;
    mvwprintw(help, ++line, 53, "Color support: ........... %s (required)", has_colors() ? "YES" : "NO");
    mvwprintw(help, ++line, 53, "Extended color support: .. %s (required)", can_change_color() ? "YES" : "NO");
    mvwprintw(help, ++line, 53, "Compiled with unicode: ... %s", USE_UNICODE == 1 ? "YES" : "NO");
    mvwprintw(help, ++line, 53, "Terminal colors: ......... %i (needs 256)", tigetnum("colors"));
    mvwprintw(help, ++line, 53, "Box characters ........... ▖,▗,▄,▌,▐,▙,▟,█ (required)");

    attron(A_ITALIC);
    mvprintw(getbegy(help) + getmaxy(help) - 2, getbegy(help) + 1, "Repository: github.com/VukanJ/tbrowser");
    attroff(A_ITALIC);

    wattron(help, COLOR_PAIR(yellow));
    box(help, 0, 0);
    wrefresh(help);
    wattroff(help, COLOR_PAIR(yellow));
    delwin(help);
}
