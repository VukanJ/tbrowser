#include "Browser.h"

bool resize_flag = false;

FileBrowser::FileBrowser() { }

FileBrowser::~FileBrowser() {
    delwin(dir_window);
}

void FileBrowser::populate(std::string filename) {
    // Get Pointers to directories, trees, and histograms
    m_filename = filename;
    m_tfile = std::unique_ptr<TFile>(TFile::Open(m_filename.c_str(), "READ"));
    if (!m_tfile || m_tfile->IsZombie()) {
        std::cerr << "File cannot be opened\n";
        exit(EXIT_FAILURE);
    }

    for (auto* KEY : *m_tfile->GetListOfKeys()) {
        TKey* key = (TKey*)KEY;
        if (strcmp(key->GetClassName(), "TTree") == 0) {
            m_trees.push_back(static_cast<TTree*>(key->ReadObj()));
        }
        else if (strcmp(key->GetClassName(), "TH1D") == 0) {
            m_histos.push_back(static_cast<TH1D*>(key->ReadObj()));
        }
        else if (strcmp(key->GetClassName(), "TH2D") == 0) {
        }
    }
    // Read branch names
    for (TTree* tree : m_trees) {
        for (auto leaf : *tree->GetListOfLeaves()) {
            m_leaves.push_back(static_cast<TLeaf*>(leaf));
        }
    }
}

Viewer::Viewer() {
    init_curses();
}

Viewer::~Viewer() {
    // Shutdown NCURSES
    delwin(main_window);
    endwin();
}

void Viewer::init_curses() {
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

    getmaxyx(stdscr, mainwin_y, mainwin_x);
    make_window();
}

void Viewer::handle_resize_events() {
    if (resize_flag) {
        getmaxyx(stdscr, mainwin_y, mainwin_x);
        make_window();
        resize_flag = false;
        endwin();
        refresh(); // Must be called after endwin
        clear();

        refresh();
    }
}

void Viewer::render() {
    box(main_window, 0, 0);
    mvwprintw(main_window, 0, 2, " ROOT TFile ");

    wrefresh(main_window);
    refresh();
}

void Viewer::make_window() {
    if (main_window != nullptr) {
        delwin(main_window);
    }
    main_window = newwin(mainwin_y, mainwin_x, 0, 0);
    refresh();
    box(main_window, 0, 0);
}

void Viewer::Main() {
    while (running) {
        handle_resize_events();
        int ch = getch();
        switch (ch) {
            case 'q': 
                running = false; 
                break;
        }
        render();
    }
}
