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
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
}

void FileBrowser::loadFile(std::string filename) {
    // Get Pointers to directories, trees, and histograms
    m_filename = filename;
    m_tfile = std::unique_ptr<TFile>(TFile::Open(m_filename.c_str(), "READ"));
    if (!m_tfile || m_tfile->IsZombie()) {
        std::cerr << "File cannot be opened\n";
        exit(EXIT_FAILURE);
    }

    for (auto* KEY : *m_tfile->GetListOfKeys()) {
        TKey* key = (TKey*)KEY;
        if (strcmp(key->GetClassName(), "TTree") == 0) {
            m_trees.push_back(static_cast<TTree*>(key->ReadObj()));
        }
        else if (strcmp(key->GetClassName(), "TH1D") == 0) {
            m_histos.push_back(static_cast<TH1D*>(key->ReadObj()));
        }
        else if (strcmp(key->GetClassName(), "TH2D") == 0) {
        }
    }
    // Read branch names
    for (TTree* tree : m_trees) {
        for (auto leaf : *tree->GetListOfLeaves()) {
            m_leaves.push_back(static_cast<TLeaf*>(leaf));
        }
    }
}

void FileBrowser::printFiles(int x, int y) {
    // mvwprintw(dir_window, x, y, ""); // TODO print tree
    int dir_size = getmaxy(dir_window);

    for (size_t i = 0; i < std::min<int>(dir_size-3, m_leaves.size()); ++i) {
        auto name = m_leaves[i]->GetName();
        if (selected_pos == i) {
            attron(A_REVERSE);
            attron(A_ITALIC);
            mvprintw(y + i + 1, x, "%.18s", m_leaves[i]->GetName());
            attroff(A_ITALIC);
            attroff(A_REVERSE);
        }
        else {
            mvprintw(y + i + 1, x, "%.18s", m_leaves[i]->GetName());
        }
    }
    box(dir_window, 0, 0);
    wrefresh(dir_window);
}

void FileBrowser::selection_down() {
    selected_pos = std::min<int>(getmaxy(dir_window) - 4, selected_pos + 1); 
}

void FileBrowser::selection_up() {
    selected_pos = std::max<int>(0, selected_pos - 1); 
}

void FileBrowser::goTop() {
    selected_pos = 0; 
}

void FileBrowser::goBottom() {
    selected_pos = m_leaves.size() - 1; 
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
    plotHistogram(*m_trees.begin(), m_leaves.at(selected_pos));
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

void FileBrowser::handleMouseClick(int y, int x) {
    // Is inside window?
    int posy, posx;
    int sizey, sizex;
    getmaxyx(dir_window, sizey, sizex);
    getbegyx(dir_window, posy, posx);
    if (x > posx && x < posx + sizex - 1 && y > posy && y < posy + sizey - 1) {
        selected_pos = y - posy - 2;
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
            plotHistogram();
            resize_flag = true;
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
