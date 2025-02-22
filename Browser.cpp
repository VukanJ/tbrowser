#include "Browser.h"
#include "AxisTicks.h"
#include <cctype>
#include <cmath>
#include <csignal>
#include <format>
#include <limits>
#include <memory>
#include <algorithm>
#include <ncurses.h>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include "definitions.h"

constexpr std::array<const char[4], 8> ascii { "▖", "▗", "▄", "▌", "▐", "▙", "▟", "█" };

std::string make_superscript(int n) {
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
    createWindow(main_window, sizey - bottom_height, sizex - menu_width, 0, menu_width);
    createWindow(dir_window, sizey - bottom_height, menu_width, 0, 0);
    createWindow(cmd_window, 3, sizex - 5 - menu_width, sizey - bottom_height + 2, 20 + 5);

refresh();
    box(dir_window, 0, 0);
    box(main_window, 0, 0);
    wrefresh(main_window);

    refreshCMDWindow();

    getmaxyx(stdscr, terminal_size_y, terminal_size_x);
}

FileBrowser::~FileBrowser() {
    if (m_tfile) {
        if (m_tfile->IsOpen()) {
            m_tfile->Close();
        }
    }
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
    signal(SIGWINCH, [](int signum) { resize_flag = true; });

    mouseinterval(0);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    start_color();
    init_pair(blue, COLOR_BLUE, COLOR_BLACK);
    init_pair(green, COLOR_GREEN, COLOR_BLACK);
    init_pair(red, COLOR_RED, COLOR_BLACK);
    init_pair(white, COLOR_WHITE, COLOR_BLACK);
    init_pair(yellow, COLOR_YELLOW, COLOR_BLACK);
    init_pair(white_on_blue, COLOR_WHITE, COLOR_BLUE);
}

void FileBrowser::loadFile(std::string filename) {
    mvprintw(1, 1, "Reading...");
    refresh();

    traverseTFile(filename);
    root_file.updateDisplayList();
}

void FileBrowser::printDirectories() {
    int x = getbegx(dir_window) + 1;
    int y = getbegy(dir_window) + 1;
    int maxlines = getmaxy(dir_window) - 2;
    int maxcols = getmaxx(dir_window);

    int entry = -menu_scroll_pos;
    auto print_entry = [this, &entry, y, x](NodeType type, std::string name, RootFile::Node* node){
        std::string entry_label;
        name = std::string(node->nesting * 2, ' ') + name;
        color col = white;
        int attr = A_NORMAL;
        switch (type) {
            case NodeType::DIRECTORY:
                if (node->directory_open) 
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

    for (const auto& [type, name, node] : root_file.displayList) {
        if (entry < 0) entry++;
        else if (entry < maxlines) {
            if (node->open) {
                print_entry(type, name, node);
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
    auto [type, name, node] = root_file.displayList.at(selected_pos + menu_scroll_pos);
    if (type == NodeType::TLEAF) {
        plotHistogram(root_file.m_trees.at(node->mother->index), 
                      root_file.m_leaves.at(node->index));
    }
}

void FileBrowser::plotHistogram(TTree* tree, TLeaf* leaf) {
    // Get window position and size
    int wx = getbegx(main_window);
    int wy = getbegy(main_window);
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

    TH1D hist("H", "H", bins_x, min, max);

    tree->Project("H", leafname);
    
    auto max_height = hist.GetAt(hist.GetMaximumBin())*1.1;

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
        mvprintw(wy + line++, wx + mainwin_x - 30, "Mean:    %.5f", hist.GetMean());
        mvprintw(wy + line++, wx + mainwin_x - 30, "Std:     %.5f", hist.GetStdDev());
        mvprintw(wy + line++, wx + mainwin_x - 30, "Bins:    %i",   bins_x);
    }

    mvprintw(wy + line++, wx + mainwin_x - 30, "Toggle key bindings <p>");
    printKeyBindings(wy + line++, wx + mainwin_x - 30);

    //mvprintw(wy + mainwin_y + 2, wx + 1, "->Draw(");
    //attron(A_REVERSE);
    //mvprintw(wy + mainwin_y + 2, wx + 8, "<d>");
    //attroff(A_REVERSE);
    //mvprintw(wy + mainwin_y + 2, wx + 11, ")");
    
    plotAxes(xaxis, wy, wx, mainwin_y, mainwin_x);

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

void FileBrowser::plotAxes(const AxisTicks& ticks, int posy, int posx, int wy, int wx) 
{
    // Clear line below plot
    move(wy, 0);
    clrtoeol();

    double xmin = ticks.min_adjusted();
    double xmax = ticks.max_adjusted();

    auto nBinsX = ticks.nticks;
    double rangeX = xmax - xmin;

    // Write numbers below axis at tick positions
    auto nchars = wx - 1;
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
            mvprintw(posy + wy, posx + 1 + charpos - ticklen / 2, "%ld", tick);
        }
        else {
            // write centered numbers below
            auto num = ticks.values_str[i];
            int ticklen = num.size();
            mvprintw(posy + wy, posx + 1 + charpos - ticklen / 2, "%s", num.c_str());
        }
        attroff(A_BOLD);
        attroff(A_ITALIC);
    }
    if (ticks.E != 0) { attroff(COLOR_PAIR(yellow)); }
    
    // Draw the tick lines
    for (int i = 0; char c : xaxis_chars) {
        mvprintw(posy + wy - 1, posx + 1 + i++, c == '+' ? "┼" : "─"); 
    }
    mvprintw(posy + wy - 1, posx + nchars, "┘");

    // Print exponent if needed
    move(wy + 1, 0);
    clrtoeol();
    if (ticks.E != 0) {
        attron(COLOR_PAIR(yellow));
#if USE_UNICODE
        std::string magnitude = std::format("x10{}", make_superscript(ticks.E));
#else
        std::string magnitude = std::format("x10^{}", make_superscript(ticks.E));
#endif
        mvprintw(posy+wy+1, posx+wx-magnitude.size(), "%s", magnitude.c_str());
        attroff(COLOR_PAIR(yellow));
    }
}

void FileBrowser::refreshCMDWindow() {
    int posx = getbegx(cmd_window) + 1;
    int posy = getbegy(cmd_window) + 1;
    // CMD hint
    attron(COLOR_PAIR(red));
    mvprintw(posy, posx - 6, "Draw(");
    attroff(COLOR_PAIR(red));

    bool error_display = false;
    if (!console.last_error.empty()) {
        error_display = true;
        attron(COLOR_PAIR(red));
        mvprintw(posy, posx, "%s", console.last_error.c_str());
        attroff(COLOR_PAIR(red));
        console.last_error.clear();
    }
    else {
        // Command state
        if (console.entering_draw_command) {
            attron(COLOR_PAIR(yellow));
            attron(A_REVERSE);
            mvprintw(posy-2, posx, " INPUT ");
            attroff(A_REVERSE);
            attroff(COLOR_PAIR(yellow));
        }
        else {
            mvprintw(posy-2, posx, "       ");
        }
    }


    // Display input
    if (!error_display) {
        if (console.current_input.empty() && !console.entering_draw_command) {
            mvprintw(posy, posx, "Press <d>");
        }
        else {
            mvprintw(posy, posx, "%s", console.current_input.c_str());
        }
    }

    if (console.entering_draw_command) {
        // Draw blinking cursor
        attron(A_REVERSE);
        if (console.curs_offset > 0) {
            // Draw blinking letter
            mvprintw(posy, posx + console.current_input.size() - console.curs_offset, "%c", console.current_input[console.current_input.size() - console.curs_offset]);
        }
        else {
            // Just draw the cursor
            attron(A_BLINK);
            mvprintw(posy, posx + console.current_input.size(), "█");
            attroff(A_BLINK);
        }
        attroff(A_REVERSE);
    }

    wattron(cmd_window, COLOR_PAIR(blue));
    box(cmd_window, 0, 0);
    wattroff(cmd_window, COLOR_PAIR(blue));
    wrefresh(cmd_window);
}

void FileBrowser::traverseTFile(TDirectory* dir, RootFile::Node* node, int depth) {
    TList* keys = dir->GetListOfKeys();
    if (!keys) return;

    for (int i = 0; i < keys->GetSize(); ++i) {
        TKey* key = (TKey*)keys->At(i);
        TObject* obj = key->ReadObj();

        if (strcmp(key->GetClassName(), "TTree") == 0) {
            TTree* tree = dynamic_cast<TTree*>(obj);
            root_file.m_trees.push_back(tree);
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::TTREE, root_file.m_trees.size() - 1, node, depth));
            readBranches(node->nodes.back().get(), tree, depth + 1);
        }
        else if (strcmp(key->GetClassName(), "TH1D") == 0) {
            root_file.m_histos_th1d.push_back(dynamic_cast<TH1D*>(obj));
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::HIST, root_file.m_histos_th1d.size() - 1, node, depth));
        }
        else if (obj->IsA()->InheritsFrom(TDirectory::Class())) {
            root_file.m_directories.push_back(dynamic_cast<TDirectory*>(obj));
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::DIRECTORY, root_file.m_directories.size() - 1, node, depth));
            traverseTFile((TDirectory*)obj, node->nodes.back().get(), depth + 1);
        }
        else {
            root_file.m_unclassified.push_back(dynamic_cast<TObject*>(obj));
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::UNKNOWN, -1, node, depth));
        }
    }
}

void FileBrowser::traverseTFile(std::string& filename) {
    m_tfile = std::unique_ptr<TFile>(TFile::Open(filename.c_str(), "READ"));
    if (!m_tfile || m_tfile->IsZombie()) {
        return;
    }

    root_file.m_directories.push_back(m_tfile.get());
    root_file.root_node.index = 0;
    root_file.root_node.type = NodeType::DIRECTORY;
    traverseTFile(m_tfile.get(), &root_file.root_node);

    // Open stuff
    root_file.root_node.setOpen(true);
    root_file.root_node.open = true; // has to be forced
}

void FileBrowser::readBranches(RootFile::Node* node, TTree* tree, int depth) {
    for (auto* leaf : *tree->GetListOfLeaves()) {
        root_file.m_leaves.push_back(static_cast<TLeaf*>(leaf));
        node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::TLEAF, root_file.m_leaves.size() - 1, node, depth));
    }
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
        console.handleInput(key);
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
        case 'd':
            console.entering_draw_command = true;
            break;
        case KEY_ENTER: case 10: // ENTER only works with RightShift+Enter
            handleMenuSelect();
            break;
        default:
            if (console.validChar(key)) {
                nonsense.push_back(key);
            }
            invalid_key = true;
            break;
    }
    refreshCMDWindow();

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

void FileBrowser::handleMenuSelect() {
    auto [type, name, node] = root_file.displayList.at(selected_pos + menu_scroll_pos);
    if (type == NodeType::DIRECTORY || type == NodeType::TTREE) {
        node->setOpen(!node->directory_open);
    }
    else if (type == NodeType::TLEAF) {
        plotHistogram(root_file.m_trees.at(node->mother->index), 
                      root_file.m_leaves.at(node->index));
    }
}

