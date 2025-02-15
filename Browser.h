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
    void loadFile(std::string filename);
    void printFiles();

    void handleInputEvent(MEVENT& mouse, int key);
    void handleResize();
    void plotHistogram();

private:
    static void init_ncurses();
    static void createWindow(WINDOW*& win, int size_y, int size_x, int pos_x, int pos_y);

    void handleMouseClick(int y, int x);
    void selection_down();
    void selection_up();
    void goTop();
    void goBottom();
    void toggleKeyBindings();
    void toggleStatsBox();
    void toggleLogy();
    void printKeyBindings(int y, int x);
    void plotHistogram(TTree*, TLeaf*);
    void plotAxes(double, double, double, double, int, int, int, int);

    WINDOW* dir_window = nullptr;
    WINDOW* main_window = nullptr;
    std::array<const char[4], 8> ascii {
        "▖", "▗", "▄", 
        "▌", "▐", "▙", 
        "▟", "█",
    };
    std::unique_ptr<TFile> m_tfile;
    std::string m_filename;
    std::vector<TTree*> m_trees;
    std::vector<TDirectory*> m_directories;
    TTree* m_active_tree = nullptr;
    std::vector<TLeaf*> m_leaves;
    std::vector<TH1D*> m_histos;
    int mainwin_x;
    int mainwin_y;
    int selected_pos = 0;
    bool showkeys = false;
    bool showstats = true;
    bool logscale = false;

    int terminal_size_y;
    int terminal_size_x;

    struct DirectoryStructure {
        struct Node {
            enum NodeType {ROOT, FOLDER, TTREE, LEAF, HIST};
            Node() : type(ROOT), index(-1) { }
            Node(NodeType nt, size_t idx) : type(nt), index(idx) { }
            NodeType type;
            size_t index = -1;
            std::vector<Node> nodes;
        } root;
    } directory;
};

#endif // BROWSER_H
