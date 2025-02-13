#include <array>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <csignal>

#include <ncurses.h>
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"

enum ASCII : std::uint8_t {
    LOWER_LEFT,
    LOWER_RIGHT,
    LOWER_HALF,
    LEFT_HALF,
    RIGHT_HALF,
    STAIRS_LEFT,
    STAIRS_RIGHT,
    FULL_BLOCK,
    VOID
};

int resize_signal = SIGWINCH;

class FileBrowser final {
public:
    FileBrowser(std::string file) : m_filename(file) {
        // Get Pointers to directories, trees, and histograms
        m_tfile = TFile::Open(m_filename.c_str(), "READ");
        for (auto* key : *m_tfile->GetListOfKeys()) {
            if (key->InheritsFrom("TTree")) {
                m_directory.emplace(dynamic_cast<TTree*>(key));
                std::cout << "NEW TTree " << key->GetName() << '\n';
            }
            if (key->InheritsFrom("TH1")) {
                m_histos.push_back(dynamic_cast<TH1*>(key));
                std::cout << "NEW TH1 " << key->GetName() << '\n';
            }
        }
    }

    void populate() {
        // Read branch names
        /* for (const auto& [ttree, branch] : m_directory) { */
        /*     for (auto* br : *ttree->GetListOfBranches()) { */
        /*         if (br->InheritsFrom("TBranch")) { */
        /*             m_directory.at(ttree).push_back(dynamic_cast<TBranch*>(br)); */
        /*         } */
        /*     } */
        /* } */
    }

private:
    std::array<const char[4], 8> ascii {
        "▖", "▗", "▄", "▌", "▐", "▙", "▟", "█"
    };
    std::string m_filename;
    TFile* m_tfile;
    std::map<TTree*, std::vector<TBranch*>> m_directory;
    std::vector<TH1*> m_histos;
};

class Viewer {
public:
    Viewer() {
        // Initialize NCURSES
        initscr(); // init
        clear();
        noecho();
        nonl(); // No NL in output
        curs_set(0);
        cbreak();
        keypad(stdscr, TRUE);  // Enable keyboard mapping
        move(0, 0);
        halfdelay(1); // TODO: Remove
        if (has_colors()) {
            start_color();

            init_pair(1, COLOR_RED, COLOR_BLACK);
            init_pair(2, COLOR_GREEN, COLOR_BLACK);
            init_pair(3, COLOR_YELLOW, COLOR_BLACK);
            init_pair(4, COLOR_BLUE, COLOR_BLACK);
            init_pair(5, COLOR_CYAN, COLOR_BLACK);
            init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(7, COLOR_WHITE, COLOR_BLACK);
        }

        main_window = newwin(LINES, COLS, 0, 0);
        refresh();
        box(main_window, 0, 0);
    }

    ~Viewer() {
        // Shutdown NCURSES
        delwin(main_window);
        endwin();
    }

    void init_window_size() {
        mainwin_y = LINES;
        mainwin_x = COLS;
        wresize(main_window, mainwin_x, mainwin_y);
        refresh();
    }

    void render() {
        box(main_window, 0, 0);
        mvwprintw(main_window, 0, 2, " Browser ");

        wrefresh(main_window);
    }

    void Main() {
        while (running) {
            int ch = getch();
            switch (ch) {
                case 'q': 
                    running = false; 
                    break;
                case KEY_RESIZE:
                    init_window_size();
                    break;
            }
            render();
        }
    }

private:
    bool running = true;
    WINDOW* main_window;
    WINDOW* directory_window;
    int mainwin_x = 39; 
    int mainwin_y = 32;
};


int main (int argc, char* argv[]) {
    setlocale(LC_ALL, "");

    signal(resize_signal, [](int signum) {
        endwin();
        refresh();
        clear();
    });

    if (argc != 2) {
        std::cerr << "Expected single argument <file>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string filename = argv[1];
    if (std::filesystem::exists(filename)) {
        FileBrowser f(filename);

        Viewer viewer;
        viewer.Main();
    }
    else {
        std::cerr << "File not found: " << filename << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
