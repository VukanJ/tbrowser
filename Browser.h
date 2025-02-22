#ifndef BROWSER_H
#define BROWSER_H

#include <ncurses.h>

#include "TFile.h"
#include "TObject.h"
#include "TTree.h"
#include "TKey.h"
#include "TLeaf.h"
#include "TH1.h"

#include "AxisTicks.h"
#include "Console.h"
#include "RootFile.h"

class FileBrowser final {
public:
    FileBrowser();
    ~FileBrowser();
    void loadFile(std::string filename);
    void printDirectories();

    void handleInputEvent(MEVENT& mouse, int key);
    void handleResize();
    void plotHistogram();

    bool isRunning() const { return is_running; };

private:
    static void initNcurses();
    static void createWindow(WINDOW*& win, int size_y, int size_x, int pos_y, int pos_x);

    void handleMenuSelect();
    void handleMouseClick(int y, int x);
    void handleInput(int key);
    bool isClickInWindow(WINDOW*&, int y, int x) const;

    // Menu control
    void selectionDown();
    void selectionUp();
    void goTop();
    void goBottom();

    // plot option toggles
    void toggleKeyBindings();
    void toggleStatsBox();
    void toggleLogy();

    void printKeyBindings(int y, int x);

    // plot commands
    void plotHistogram(TTree*, TLeaf*);
    void plotHistogram(const Console::DrawArgs&);
    void plotAxes(const AxisTicks&, int, int, int, int);

    // Window refreshing
    void refreshCMDWindow();
    Console console;

    void traverseTFile(TDirectory*, RootFile::Node*, int depth=0);
    void traverseTFile(std::string& filename);
    void readBranches(RootFile::Node*, TTree*, int depth);

    // Ncurses
    WINDOW* dir_window = nullptr;
    WINDOW* main_window = nullptr;
    WINDOW* cmd_window = nullptr;

    int mainwin_x;
    int mainwin_y;
    int terminal_size_y;
    int terminal_size_x;
    int menu_width = 20;
    int bottom_height = 6;

    int selected_pos = 0;
    int menu_scroll_pos = 0;

    // Toggles
    bool showkeys = false;
    bool showstats = true;
    bool logscale = false;
    bool is_running = true; // false if program should end

    // Collect user input, transfer to input mode if too much nonsense is entered
    std::string nonsense;

    // ROOT
    std::unique_ptr<TFile> m_tfile;
    RootFile root_file;

    // State
    RootFile::Node* last_drawn_leaf = nullptr;
};

#endif // BROWSER_H
