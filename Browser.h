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
    void populate(std::string filename);
    void make_window();

    void printFiles(int lines, int cols, int x, int y);

    void select_down() { selected_pos++; }
    void select_up() { selected_pos--; }
    void goTop() { selected_pos = 0; }
    void goBottom() { selected_pos = m_leaves.size() - 1; }
    void toggleKeyBindings() { showkeys = !showkeys; }
    void toggleStatsBox() { showstats = !showstats; } 
    void toggleLogy() { logscale = !logscale; } 
    void plotHistogram(WINDOW*& win);

    void handleMouseClick(int y, int x);
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
    int selected_pos = 3;
    bool showkeys = false;
    bool showstats = true;
    bool logscale = false;
};

#endif // BROWSER_H
