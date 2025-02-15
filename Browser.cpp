#include "Browser.h"
#include <csignal>
#include <iostream>
#include <memory>
#include <algorithm>
#include <ncurses.h>
#include <string>
#include <unordered_map>

enum ASCII : std::uint8_t {
    LOWER_LEFT,
    LOWER_RIGHT,
    LOWER_HALF,
    LEFT_HALF,
    RIGHT_HALF,
    STAIRS_LEFT,
    STAIRS_RIGHT,
    FULL_BLOCK,
    VOID,
};

enum ASCII_code : std::uint8_t {
    C_LOWER_LEFT   = 0b1000,
    C_LOWER_RIGHT  = 0b0100,
    C_LOWER_HALF   = 0b1100,
    C_LEFT_HALF    = 0b1010,
    C_RIGHT_HALF   = 0b0101,
    C_STAIRS_LEFT  = 0b1110,
    C_STAIRS_RIGHT = 0b1101,
    C_FULL_BLOCK   = 0b1111,
    C_VOID         = 0b0000,
};

std::unordered_map<ASCII_code, ASCII> ascii_map = {
    {C_VOID, VOID},
    {C_LOWER_LEFT, LOWER_LEFT},
    {C_LOWER_RIGHT, LOWER_RIGHT},
    {C_LOWER_HALF, LOWER_HALF},
    {C_LEFT_HALF, LEFT_HALF},
    {C_RIGHT_HALF, RIGHT_HALF},
    {C_STAIRS_LEFT, STAIRS_LEFT},
    {C_STAIRS_RIGHT, STAIRS_RIGHT},
    {C_FULL_BLOCK, FULL_BLOCK},
};

volatile bool resize_flag = false;

FileBrowser::FileBrowser() {
    init_ncurses();
    int sizex = getmaxx(stdscr);
    int sizey = getmaxy(stdscr);;
    createWindow(main_window, sizey - 6, sizex - 20, 20, 0);
    createWindow(dir_window, sizey - 6, 20, 0, 0);

    refresh();
    box(main_window, 0, 0);
    wrefresh(main_window);

    getmaxyx(stdscr, terminal_size_y, terminal_size_x);
}

FileBrowser::~FileBrowser() {
    if (m_tfile) {
        if (m_tfile->IsOpen()) {
            m_tfile->Close();
        }
    }

    delwin(main_window);
    delwin(dir_window);
    endwin();
}

void FileBrowser::init_ncurses() {
    // Init NCURSES
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    // halfdelay(20);
    signal(SIGWINCH, [](int signum) { resize_flag = true; });

    mouseinterval(0);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    start_color();
    init_pair(blue, COLOR_BLUE, COLOR_BLACK);
    init_pair(green, COLOR_GREEN, COLOR_BLACK);
    init_pair(red, COLOR_RED, COLOR_BLACK);
    init_pair(white, COLOR_WHITE, COLOR_BLACK);
    init_pair(yellow, COLOR_YELLOW, COLOR_BLACK);
}

void FileBrowser::loadFile(std::string filename) {
    debug_traverse(filename);
    root_file.updateDisplayList();
}

void FileBrowser::printDirectories() {
    int x = getbegx(dir_window) + 1;
    int y = getbegy(dir_window) + 1;

    int entry = 0;
    auto print_entry = [this, &entry, y, x](NodeType type, const std::string name, RootFile::Node* node){
        std::string entry_label;
        color col = white;
        int attr = A_NORMAL;
        switch (type) {
            case NodeType::DIRECTORY: 
                if (node->directory_open) {
                    entry_label = " " + name;
                }
                else {
                    entry_label = " " + name;
                }
                col = yellow;
                break;
            case NodeType::LEAF:    entry_label = " " + name; break;
            case NodeType::TTREE:   entry_label = " " + name; col = green; attr = A_BOLD; break;
            case NodeType::HIST:    entry_label = " " + name; col = blue; attr = A_ITALIC; break;
            case NodeType::UNKNOWN: entry_label = "? " + name; col = red; break;
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

    for (const auto& [type, name, node] : root_file.displayList) {
        if (node->displayInMenu()) {
            print_entry(type, name, node);
        }
    }

    box(dir_window, 0, 0);
    wrefresh(dir_window);
}

void FileBrowser::selection_down() {
    mvprintw(30, 30, "MENU_LEGTH %i", root_file.menuLength());
    selected_pos = std::min<int>(std::min<int>(root_file.menuLength() - 1, getmaxy(dir_window) - 4), selected_pos + 1); 
}

void FileBrowser::selection_up() {
    selected_pos = std::max<int>(0, selected_pos - 1); 
}

void FileBrowser::goTop() {
    selected_pos = 0; 
}

void FileBrowser::goBottom() {
    selected_pos = root_file.menuLength() - 1;
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
    plotHistogram(m_active_tree, m_leaves.at(selected_pos));
}

void FileBrowser::plotHistogram(TTree* tree, TLeaf* leaf) {
    // Get window position and size
    int wx = 0;
    int wy = 0;
    getbegyx(main_window, wy, wx);
    getmaxyx(main_window, mainwin_y, mainwin_x);
    box(main_window, 0, 0);

    // Get bounds
    const char* leafname = leaf->GetName();
    auto bins_x = 2*(mainwin_x - 2); // 20=Width of browser, 4 border characters. x2: 2bins per character
    auto bins_y = 2*(mainwin_y - 2);
    auto min = tree->GetMinimum(leafname);
    auto max = tree->GetMaximum(leafname);
    if (min == max) {
        // Handle single value histograms
        min = min - 1;
        max = min + 1;
    }
    TH1F hist("H", "H", bins_x, max, min);

    tree->Project("H", leafname);
    
    auto max_height = hist.GetAt(hist.GetMaximumBin())*1.1;

    double binwidth = (max - min) / bins_x;
    double pixel_y = max_height / bins_y;

    if (logscale) {
        max_height = log(max_height) + 1.0f;  //+1 order of base magnitude for plotting
        pixel_y = max_height / bins_y;
    }
    
    // Draw ASCII art
    attron(COLOR_PAIR(1));
    for (int x = 0; x < bins_x / 2; x++) {
        auto cl = hist.GetBinContent(2 * x);
        auto cr = hist.GetBinContent(2 * x + 1);
        if (logscale) {
            cl = log(cl);
            cr = log(cr);
        }
        for (int y = 0; y < bins_y / 2; ++y) {
            // Check if ascii character is filled
            std::uint8_t probe = ASCII_code::C_VOID;
            probe |= ((2*y + 0) * pixel_y < cl) << 3;
            probe |= ((2*y + 0) * pixel_y < cr) << 2;
            probe |= ((2*y + 1) * pixel_y < cl) << 1;
            probe |= ((2*y + 1) * pixel_y < cr);
            if (probe == ASCII_code::C_VOID) {
                // fill rest with blanks, prevents overdraw...
                for (int f = y; f < bins_y / 2; ++f) {
                    mvprintw(wy+mainwin_y-2-f, wx+1+x, " ");
                }
                break;
            }
            try {
                auto print = ascii[ascii_map.at(static_cast<ASCII_code>(probe))];
                mvprintw(wy+mainwin_y-2-y, wx+1+x, "%s", print);
            }
            catch(...) {
                mvprintw(wy+mainwin_y-2-y, wx+1+x, "%i", probe);
            }

        }
    }
    attroff(COLOR_PAIR(1));

    // Various annotations

    // Plot Title
    box(main_window, 0, 0);
    wrefresh(main_window);
    attron(A_ITALIC);
    attron(A_UNDERLINE);
    if (logscale) {
        mvprintw(0, wx+2, "┤ log(%s) ├", leaf->GetTitle());
    }
    else {
        mvprintw(0, wx+2, "┤ %s ├", leaf->GetTitle());
    }
    attroff(A_UNDERLINE);
    attroff(A_ITALIC);

    // Plot stats
    int line = 1;
    if (showstats) {
        mvprintw(wy + line++, wx + mainwin_x - 30, "Entries: %i", (int)hist.GetEntries());
        mvprintw(wy + line++, wx + mainwin_x - 30, "Mean:    %.2f ± %.2f", hist.GetMean(), hist.GetMeanError());
        mvprintw(wy + line++, wx + mainwin_x - 30, "Std:     %.2f ± %.2f", hist.GetStdDev(), hist.GetStdDevError());
    }

    mvprintw(wy + line++, wx + mainwin_x - 30, "Toggle key bindings <p>");
    printKeyBindings(wy + line++, wx + mainwin_x - 30);

    mvprintw(wy + mainwin_y + 1, wx + 1, "->Draw(");
    attron(A_REVERSE);
    mvprintw(wy + mainwin_y + 1, wx + 8, "<d>");
    attroff(A_REVERSE);
    mvprintw(wy + mainwin_y + 1, wx + 11, ")");

    plotAxes(min, max, 0, max_height * 1.1, wy, wx, mainwin_y, mainwin_x);
    refresh();

}

void FileBrowser::printKeyBindings(int y, int x) {
    int line = y + 2;
    if (showkeys) {
        attron(A_UNDERLINE);
        mvprintw(line++, x, "</> Search branch");
        mvprintw(line++, x, "<s> Toggle stats box");
        mvprintw(line++, x, "<d> Enter draw command");
        mvprintw(line++, x, "<l> Toggle log y");
        mvprintw(line++, x, "<g> Go to top");
        mvprintw(line++, x, "<G> Go to bottom");
        mvprintw(line++, x, "<q/Esc> Quit");
        attroff(A_UNDERLINE);
    }
}

void FileBrowser::plotAxes(double xmin, double xmax, double ymin, double ymax, 
                           int posy, int posx, int wy, int wx) 
{
    std::vector<char> xaxis(wx-1, '-');

    auto xwidth = xmax - xmin;
    // mvprintw(30, 30, std::to_string(xwidth).c_str());
    int nticks = 5;
    for (double xt = xmin; xt <= xmax; xt += xwidth / nticks) {
        int offset = ((xt - xmin) / xwidth) * wx;
        xaxis[std::min<int>(offset, xaxis.size() - 1)] = '+';
    }

    for (int i = 0; char c : xaxis) {
        if (xaxis[i] == '-') {
            mvprintw(posy + wy - 1, posx + 1 + i, "─");
        }
        else if (xaxis[i] == '+') {
            mvprintw(posy + wy - 1, posx + 1 + i, "┼");
        }

        i++;
    }
}

void FileBrowser::debug_listdir(TDirectory* dir, RootFile::Node* node) {
    TList* keys = dir->GetListOfKeys();
    if (!keys) return;

    for (int i = 0; i < keys->GetSize(); ++i) {
        TKey* key = (TKey*)keys->At(i);
        TObject* obj = key->ReadObj();

        if (strcmp(key->GetClassName(), "TTree") == 0) {
            root_file.m_trees.push_back(dynamic_cast<TTree*>(obj));
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::TTREE, root_file.m_trees.size() - 1, node));
        }
        else if (strcmp(key->GetClassName(), "TH1D") == 0) {
            root_file.m_histos_th1d.push_back(dynamic_cast<TH1D*>(obj));
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::HIST, root_file.m_histos_th1d.size() - 1, node));
        }
        else if (obj->IsA()->InheritsFrom(TDirectory::Class())) {
            root_file.m_directories.push_back(dynamic_cast<TDirectory*>(obj));
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::DIRECTORY, root_file.m_directories.size() - 1, node));
            debug_listdir((TDirectory*)obj, node->nodes.back().get());
        }
        else {
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::UNKNOWN, -1, node));
        }
    }
}

void FileBrowser::debug_traverse(std::string& filename) {
    m_tfile = std::unique_ptr<TFile>(TFile::Open(filename.c_str(), "READ"));
    if (!m_tfile || m_tfile->IsZombie()) {
        return;
    }

    root_file.m_directories.push_back(m_tfile.get());
    root_file.root_node.index = 0;
    root_file.root_node.type = NodeType::DIRECTORY;
    debug_listdir(m_tfile.get(), &root_file.root_node);

    if (root_file.m_trees.size() > 0) {
        m_active_tree = root_file.m_trees.back();
    }
}

void FileBrowser::handleMouseClick(int y, int x) {
    // Is inside window?
    int posy, posx;
    int sizey, sizex;
    getmaxyx(dir_window, sizey, sizex);
    getbegyx(dir_window, posy, posx);
    if (x > posx && x < posx + sizex - 1 && y > posy && y < posy + sizey - 1) {
        selected_pos = y - posy - 1;
    }
}

void FileBrowser::handleInputEvent(MEVENT& mouse_event, int key) {
    switch (key) {
        case KEY_DOWN:
            selection_down();
            break;
        case KEY_UP:
            selection_up();
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
        case 'p':
            toggleKeyBindings();
            plotHistogram();
            break;
        case 's':
            toggleStatsBox();
            plotHistogram();
            break;
        case 'l':
            toggleLogy();
            plotHistogram();
            break;
        case KEY_ENTER: case 10: // ENTER only works with RightShift+Enter
            handleMenuSelect();
            break;
        case KEY_MOUSE:
            if (getmouse(&mouse_event) == OK) {
                if (mouse_event.bstate == BUTTON1_PRESSED) {
                    handleMouseClick(mouse_event.y, mouse_event.x);
                }
                else if (mouse_event.bstate == BUTTON4_PRESSED) {
                    selection_up();
                }
                else if (mouse_event.bstate == BUTTON5_PRESSED) {
                    selection_down();
                }
            }
            break;
    }
}

void FileBrowser::handleResize() {
    resize_flag = false;
    int sizex = getmaxx(stdscr);
    int sizey = getmaxy(stdscr);
    getmaxyx(stdscr, sizey, sizex);
    // !!! May be false positive, check
    if (sizex != terminal_size_x || sizey != terminal_size_y) {
        // mvprintw(getmaxy(dir_window), 0, "RESIZE %i,%i", sizey, sizex);
        terminal_size_x = sizex;
        terminal_size_y = sizey;
        endwin();
        refresh();
        createWindow(main_window, terminal_size_y - 6, terminal_size_x - 20, 20, 0);
        createWindow(dir_window, terminal_size_y - 6, 20, 0, 0);
    }
    else {
        // mvprintw(getmaxy(dir_window), 0, " FAKERESIZE %i,%i", sizey, sizex);
    }
}

void FileBrowser::createWindow(WINDOW*& win, int size_y, int size_x, int pos_x, int pos_y) {
    if (win) {
        delwin(win);
    }
    win = newwin(size_y, size_x, pos_y, pos_x);
    refresh();
    box(win, 0, 0);
    wrefresh(win);
}


void FileBrowser::handleMenuSelect() {
    auto [type, name, node] = root_file.displayList.at(selected_pos);
    switch (type) {
        case NodeType::DIRECTORY:
            node->directory_open = !node->directory_open;
            root_file.updateDisplayList();
            break;
    }
}

void FileBrowser::RootFile::updateDisplayList(Node* mothernode, std::string prefix) {
    for (auto& node : mothernode->nodes) {
        switch (node->type) {
            case NodeType::DIRECTORY:
                displayList.emplace_back(node->type, m_directories[node->index]->GetName(), node.get());
                updateDisplayList(node.get(), prefix + " ");
                break;
            case NodeType::TTREE:
                displayList.emplace_back(node->type, m_trees[node->index]->GetName(), node.get());
                break;
            case NodeType::HIST:
                displayList.emplace_back(node->type, m_histos_th1d[node->index]->GetName(), node.get());
                break;
            default: break;
        }
    }
}

void FileBrowser::RootFile::updateDisplayList() {
    // Make flat file structure list for quick redraw
    displayList.clear();
    displayList.emplace_back(root_node.type, m_directories[root_node.index]->GetName(), &root_node);
    updateDisplayList(&root_node, "");
}

std::optional<FileBrowser::RootFile::MenuItem> FileBrowser::RootFile::getEntry(int i) {
    int entry = 0;
    for (size_t j = 0; j < displayList.size(); ++j) {
        if (std::get<2>(displayList[j])->displayInMenu()) {
            if (entry == i) {
                return displayList[j];
            }
            entry++;
        }
    }
    return std::nullopt;
}

int FileBrowser::RootFile::menuLength() {
    int length = 0;
    for (const auto& [a, b, node] : displayList) {
        if (node->displayInMenu()) {
            length++;
        }
    }
    return length;
}

bool FileBrowser::RootFile::Node::displayInMenu() {
    // return true if this node is visible (its containing directory is open)
    if (mother == nullptr) {
        return true; // Root always visible
    }
    if (type == NodeType::DIRECTORY && !directory_open) {
        return false;
    }
    else {
        return mother->displayInMenu();
    }
}
