#include "Browser.h"
#include "RtypesCore.h"
#include <iostream>
#include <format>
#include <memory>
#include <algorithm>
#include <ncurses.h>
#include <thread>
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

FileBrowser::FileBrowser() { }

FileBrowser::~FileBrowser() { }

void FileBrowser::populate(std::string filename) {
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

void FileBrowser::printFiles(int lines, int cols, int x, int y) {
    // mvwprintw(dir_window, x, y, ""); // TODO print tree
    for (size_t i = 0; i < std::min<int>(mainwin_y-9, m_leaves.size()); ++i) {
        auto name = m_leaves[i]->GetName();
        if (selected == i) {
            // attron(A_REVERSE);
            attron(A_ITALIC);
            attron(A_BOLD);
            mvprintw(y + i + 1, x, "%.18s", m_leaves[i]->GetName());
            attroff(A_BOLD);
            attroff(A_ITALIC);
            // attroff(A_REVERSE);
        }
        else {
            mvprintw(y + i + 1, x, "%.18s", m_leaves[i]->GetName());
        }
    }
    box(dir_window, 0, 0);
    wrefresh(dir_window);
}

void FileBrowser::plotHistogram(WINDOW*& win) {
    plotHistogram(win, *m_trees.begin(), m_leaves.at(selected));
}

void FileBrowser::plotHistogram(WINDOW*& win, TTree* tree, TLeaf* leaf) {
    int debug = 1;
    // Get window position and size
    int wx = 0;
    int wy = 0;
    getbegyx(win, wy, wx);
    getmaxyx(win, mainwin_y, mainwin_x);
    wclear(win);
    box(win, 0, 0);

    attron(A_BLINK);
    mvprintw(wy + mainwin_y / 2, wx + mainwin_x / 2, "Please Wait...");
    attroff(A_BLINK);
    wrefresh(win);
    refresh();

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
    // Draw ASCII art
    
    attron(COLOR_PAIR(1));
    for (int x = 0; x < bins_x / 2; x++) {
        auto cl = hist.GetBinContent(2 * x);
        auto cr = hist.GetBinContent(2 * x + 1);
        for (int y = 0; y < bins_y / 2; ++y) {
            // Check if ascii character is filled
            std::uint8_t probe = ASCII_code::C_VOID;
            probe |= ((2*y - 0.5)* pixel_y <= cl) << 3;
            probe |= ((2*y - 0.5)* pixel_y <= cr) << 2;
            probe |= ((2*y + 0.5) * pixel_y <= cl) << 1;
            probe |= ((2*y + 0.5) * pixel_y <= cr);
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

    // Plot stats
    mvprintw(wy + 1, wx + mainwin_x - 30, "Entries: %i", (int)hist.GetEntries());
    mvprintw(wy + 2, wx + mainwin_x - 30, "Mean:    %.2f ± %.2f", hist.GetMean(), hist.GetMeanError());
    mvprintw(wy + 3, wx + mainwin_x - 30, "Std:     %.2f ± %.2f", hist.GetStdDev(), hist.GetStdDevError());
    box(win, 0, 0);
    refresh();

    /* for (int y = 0, i = 0; y < bins_y / 2; y++) { */
    /*     for (int x = 0; x < bins_x / 2; x++) { */
    /*         mvprintw(wy+mainwin_y - y - 2, wx + x + 1, ascii[rand()%8]); */
    /*     } */
    /* } */

}
