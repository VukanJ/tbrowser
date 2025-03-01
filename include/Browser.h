#ifndef BROWSER_H
#define BROWSER_H

#include <ncurses.h>

#include "TTree.h"
#include "TLeaf.h"
#include "TH1.h"
#include "TH2.h"

#include "AxisTicks.h"
#include "Console.h"
#include "definitions.h"
#include "RootFile.h"
#include <nlohmann/json.hpp>

using JSON = nlohmann::json;

class FileBrowser final {
public:
    FileBrowser();
    ~FileBrowser();

    void loadFile(std::string filename);
    void printDirectories();

    void handleInputEvent(MEVENT& mouse, int key);
    void handleResize(bool force=false);
    void plotHistogram();

    bool isRunning() const;

private:
    static void initNcurses();
    static void createWindow(WINDOW*& win, int size_y, int size_x, int pos_y, int pos_x);

    // Persistence
    void loadSettings();
    void saveSettings();

    // Input
    void handleMenuSelect();
    void handleMouseClick(int y, int x);
    void handleInput(int key);
    static bool isClickInWindow(WINDOW*&, int y, int x);

    // Menu control
    void selectionDown();
    void selectionUp();
    void goTop();
    void goBottom();

    // plot option toggles
    void toggleStatsBox();
    void toggleLogy();

    // plot commands
    void plotHistogram(TTree*, TLeaf*);
    void plotHistogram(const Console::DrawArgs&);
    void plot2DHistogram(const Console::DrawArgs&);
    void plotAxes(const AxisTicks&, int, int, int, int, bool force_range);
    void plotCanvasAnnotations(TH1* hist, int winy, int winx);
    void plotCanvasAnnotations(TH2* hist, int winy, int winx);
    void plotASCIIHistogram(int winy, int winx, TH1D* hist, int binsy, int binsx) const;
    void plotASCIIHistogram2D(int winy, int winx, TH2D* hist, int binsy, int binsx);
    void drawEssentials();

    // Window refreshing
    void initAllWindows();
    void refreshCMDWindow();
    void helpWindow();
    void drawColorWindow();
    TTree* getActiveTTree();
    Console console;

    int getBinsx();
    int getBinsy();
    void toggleBlockMode();

    JSON settings_json;

    // Ncurses
    WINDOW* dir_window = nullptr;
    WINDOW* main_window = nullptr;
    WINDOW* cmd_window = nullptr;
    std::filesystem::path dotpath;

    int mainwin_x;
    int mainwin_y;
    int terminal_size_y;
    int terminal_size_x;

    int menu_width = 20;
    int bottom_height = 7;

    int selected_pos = 0;
    int menu_scroll_pos = 0;

    // Toggles
    bool showstats = true;
    bool logscale = false;
    bool is_running = true; // false if program should end
    
    int blockmode = 2;

    // Collect user input, transfer to input mode if too much nonsense is entered
    std::string nonsense;

    // ROOT
    RootFile root_file;

    bool skipDraw = false;

    struct ColorWindow {
        ColorWindow() = default;
        ~ColorWindow();

        void init(); // Must be delayed

        void render();
        bool show = false;
        TermColor selected = col_blue;
        TermColor selectFromMouseClick(int y, int x);
        WINDOW* color_window = nullptr;
    } colorWindow;
    
};

#endif // BROWSER_H
