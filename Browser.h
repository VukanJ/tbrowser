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
    FileBrowser();
    ~FileBrowser();
    void populate(std::string filename);
    void make_window();

    void printFiles(int lines, int cols, int x, int y);

    void select_down() {
        selected++;
    }
    void select_up() {
        selected--;
    }

private:
    WINDOW* dir_window = nullptr;
    std::array<const char[4], 8> ascii {
        "▖", "▗", "▄", 
        "▌", "▐", "▙", 
        "▟", "█"
    };
    std::unique_ptr<TFile> m_tfile;
    std::string m_filename;
    std::vector<TTree*> m_trees;
    std::vector<TLeaf*> m_leaves;
    std::vector<TH1D*> m_histos;
    int mainwin_x;
    int mainwin_y;
    int selected = 3;
};

#endif // BROWSER_H
