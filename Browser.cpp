#include "Browser.h"
#include "RtypesCore.h"
#include <iostream>
#include <format>
#include <memory>
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

bool resize_flag = false;

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
    mvprintw(x, y, ""); // TODO print tree
    for (size_t i = 0; i < m_leaves.size(); ++i) {
        auto name = std::string(" ") + m_leaves[i]->GetName();
        if (selected == i) {
            attron(A_REVERSE);
            mvprintw(y + i + 1, x, "%.20s", name.c_str());
            attroff(A_REVERSE);
        }
        else {
            mvprintw(y + i+1, x, "%.20s", name.c_str());
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
    int wx = 0;
    int wy = 0;
    getbegyx(win, wy, wx);
    // mvprintw(debug++, wx, "Plotting %s %s", leaf->GetName(), leaf->GetTitle());
    getmaxyx(win, mainwin_y, mainwin_x);
    box(win, 0, 0);
    wrefresh(win);
    wclear(win);
    auto bins_x = 2*(mainwin_x - 2); // 20=Width of browser, 4 border characters. x2: 2bins per character
    auto bins_y = 2*(mainwin_y - 2);
    auto min = tree->GetMinimum(leaf->GetName());
    auto max = tree->GetMaximum(leaf->GetName());
    Long64_t nEntries = tree->GetEntries();
    // mvprintw(debug++, wx, "winsize %i %i", mainwin_x, mainwin_y);
    // mvprintw(debug++, wx, "bins %i %i", bins_x, bins_y);
    // mvprintw(debug++, wx, "minmax %f %f", min, max);
    if (min == max) {
        min = min - 1;
        max = min + 1;
    }
    std::unique_ptr<TH1F> hist = std::make_unique<TH1F>("H", "H", bins_x, max, min);

    tree->Project("H", leaf->GetName());
    
    auto max_height = hist->GetAt(hist->GetMaximumBin())*1.1;
    // mvprintw(debug++, wx, "MAXHEIGHT %f", max_height);
    
    double binwidth = (max - min) / bins_x;
    double pixel_y = max_height / bins_y;
    // Draw ASCII art
    
    attron(COLOR_PAIR(1));
    for (int x = 0; x < bins_x / 2; x++) {
        auto cl = hist->GetBinContent(2 * x);
        auto cr = hist->GetBinContent(2 * x + 1);
        for (int y = 0; y < bins_y / 2; ++y) {
            // Check if ascii character is filled
            std::uint8_t probe = ASCII_code::C_VOID;
            probe |= ((2*y - 1)* pixel_y <= cl) << 3;
            probe |= ((2*y - 1)* pixel_y <= cr) << 2;
            probe |= ((2*y + 0) * pixel_y <= cl) << 1;
            probe |= ((2*y + 0) * pixel_y <= cr);
            if (probe == ASCII_code::C_VOID) break;

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

    /* for (int y = 0, i = 0; y < bins_y / 2; y++) { */
    /*     for (int x = 0; x < bins_x / 2; x++) { */
    /*         mvprintw(wy+mainwin_y - y - 2, wx + x + 1, ascii[rand()%8]); */
    /*     } */
    /* } */

}
