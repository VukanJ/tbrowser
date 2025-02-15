#ifndef BROWSER_H
#define BROWSER_H

#include <ncurses.h>

#include "TFile.h"
#include "TTree.h"
#include "TKey.h"
#include "TLeaf.h"
#include "TH1.h"

class FileBrowser final {
public:
    FileBrowser(WINDOW*& directory_window);
    ~FileBrowser();
    void loadfile(std::string filename);
    void printFiles(int lines, int cols, int x, int y);
    void selection_down();
    void selection_up();
    void goTop();
    void goBottom();
    void toggleKeyBindings();
    void toggleStatsBox();
    void toggleLogy();
    void handleMouseClick(int y, int x);
    void plotHistogram(WINDOW*& win);
private:
    void printKeyBindings(int y, int x);
    void plotHistogram(WINDOW*& win, TTree*, TLeaf*);
    void plotAxes(double, double, double, double, int, int, int, int);

    WINDOW* dir_window = nullptr;
    std::array<const char[4], 8> ascii {
        "▖", "▗", "▄", 
        "▌", "▐", "▙", 
        "▟", "█",
    };
    std::unique_ptr<TFile> m_tfile;
    std::string m_filename;
    std::vector<TTree*> m_trees;
    std::vector<TLeaf*> m_leaves;
    std::vector<TH1D*> m_histos;
    int mainwin_x;
    int mainwin_y;
    int selected_pos = 0;
    bool showkeys = false;
    bool showstats = true;
    bool logscale = false;
};

#endif // BROWSER_H
