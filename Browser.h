#ifndef BROWSER_H
#define BROWSER_H

#include <ncurses.h>
#include <optional>
#include <unordered_set>

#include "AxisTicks.h"
#include "TFile.h"
#include "TObject.h"
#include "TTree.h"
#include "TKey.h"
#include "TLeaf.h"
#include "TH1.h"

class FileBrowser final {
public:
    FileBrowser();
    ~FileBrowser();
    void loadFile(std::string filename);
    void printDirectories();

    void handleInputEvent(MEVENT& mouse, int key);
    void handleResize();
    void plotHistogram();

private:
    enum color {
        blue=1, green, red, white, yellow, white_on_blue
    };
    static void init_ncurses();
    static void createWindow(WINDOW*& win, int size_y, int size_x, int pos_y, int pos_x);

    void handleMenuSelect();
    void handleMouseClick(int y, int x);
    void handleConsoleInput(int key);

    // Menu control
    void selection_down();
    void selection_up();
    void goTop();
    void goBottom();

    // plot option toggles
    void toggleKeyBindings();
    void toggleStatsBox();
    void toggleLogy();

    void printKeyBindings(int y, int x);

    // plot commands
    void plotHistogram(TTree*, TLeaf*);
    void plotAxes(const AxisTicks&, int, int, int, int);

    // Window refreshing
    void refresh_cmd_window();
    struct InputConsoleState {
        InputConsoleState();
        std::unordered_set<char> allowed_chars;
        std::vector<std::string> command_buffer;
        std::string current_input;
        int curs_offset = 0;
    } console;

    WINDOW* dir_window = nullptr;
    WINDOW* main_window = nullptr;
    WINDOW* cmd_window = nullptr;

    std::unique_ptr<TFile> m_tfile;
    int mainwin_x;
    int mainwin_y;
    int selected_pos = 0;
    int menu_scroll_pos = 0;
    bool showkeys = false;
    bool showstats = true;
    bool logscale = false;
    bool entering_draw_command = false; // Key press is letter

    int terminal_size_y;
    int terminal_size_x;

    int menu_width = 20;
    int bottom_height = 6;

    enum class NodeType { DIRECTORY, TTREE, TLEAF, HIST, UNKNOWN };
    class RootFile {
    public:
        struct Node {
            Node() : type(NodeType::UNKNOWN), index(-1) { }
            Node(NodeType nt, int idx, Node* mot, int nest) 
                : type(nt), index(idx), mother(mot), nesting(nest) { }
            NodeType type;
            int index = -1; // Logical pointer to storage
            Node* mother; // So leaves know their tree
            std::vector<std::unique_ptr<Node>> nodes;

            bool directory_open = false;  // By default, open all directories
            bool open = false;
            void setOpen(bool, int recurse=0);
            int nesting = 0;
        } root_node;

        std::vector<TDirectory*> m_directories;
        std::vector<TTree*> m_trees;
        std::vector<TLeaf*> m_leaves;
        std::vector<TH1D*> m_histos_th1d;
        std::vector<TObject*> m_unclassified;

        std::string toString(Node*);

        using MenuItem = std::tuple<NodeType, std::string, Node*>;
        void updateDisplayList(Node*, int nesting=0);
        void updateDisplayList();
        std::optional<MenuItem> getEntry(int);
        int menuLength();
        std::vector<MenuItem> displayList;

    } root_file;

    void traverse_tfile(TDirectory*, RootFile::Node*, int depth=0);
    void traverse_tfile(std::string& filename);
    void readBranches(RootFile::Node*, TTree*, int depth);
};

#endif // BROWSER_H
