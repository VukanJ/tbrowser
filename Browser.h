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
    void render();

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
};

class Viewer {
public:
    Viewer();
    ~Viewer();

    void init_curses();
    void handle_resize_events();
    void render();
    void Main();

    void openFile(std::string filename);

private:
    void make_window();

    FileBrowser fileBrowser;
    WINDOW* main_window = nullptr;
    bool running = true;
    std::string m_filename;
    int mainwin_x;
    int mainwin_y;
};

#endif // BROWSER_H
