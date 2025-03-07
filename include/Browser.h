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
#include "Menu.h"
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

    // plot option toggles
    void toggleStatsBox();
    void toggleLogy();

    // plot commands
    void plotHistogram(TTree*, TLeaf*);
    void plotHistogram(const Console::DrawArgs&);
    void plot2DHistogram(const Console::DrawArgs&);
    void plotXAxis(AxisTicks&, bool force_range);
    void plotYAxis(AxisTicks&, bool force_range);
    void plotCanvasAnnotations(TH1* hist);
    void plotCanvasAnnotations(TH2* hist);
    void plotASCIIHistogram(TH1D* hist, int binsy, int binsx) const;
    void plotASCIIHistogram2D(TH2D* hist, int binsy, int binsx);
    void drawEssentials();

    // Window refreshing
    void initAllWindows();
    void refreshCMDWindow();
    void helpWindow();
    void drawColorWindow();
    TTree* getActiveTTree();
    void makeSpaceForYaxis(int s);
    Console console;

    int getBinsx() const ;
    int getBinsy() const ;
    void toggleBlockMode();

    JSON settings_json;

    // Ncurses
    WINDOW* dir_window = nullptr;
    WINDOW* main_window = nullptr;
    WINDOW* cmd_window = nullptr;
    std::filesystem::path dotpath;

    int mainwin_x;
    int mainwin_y;

    int menu_width = 20;
    int bottom_height = 7;
    int yaxis_spacing = 5;

    constexpr static float top_hist_clear = 1.1; // Extra space

    // Toggles
    bool showstats = true;
    bool logscale = false;
    bool is_running = true; // false if program should end
    
    int blockmode = 2;

    // Collect user input, transfer to input mode if too much nonsense is entered
    std::string nonsense;

    // ROOT
    RootFile root_file;
    
    // skip next directory draw
    bool skipDraw = false;

    struct ColorPickerWindow {
        ColorPickerWindow() = default;
        ~ColorPickerWindow();

        void init(); // Must be delayed
        void render() const;
        TermColor selectFromMouseClick(int y, int x);

        bool show = false;
        TermColor selected = col_blue;
        WINDOW* color_window = nullptr;
    } colorWindow;

    struct SearchMode {
        bool isActive = false;
        std::string input;
        static bool isBranchChar(int key);
    } searchMode;
    void updateSearchResults();
    void setAllSearchResultTrue();

    Menu object_menu;
};

#endif // BROWSER_H
