#include <algorithm>
#include <clocale>
#include <cstdint>
#include <functional>
#include <iostream>
#include <filesystem>
#include <iterator>
#include <string>
#include <csignal>

#include <ncurses.h>
#include "Browser.h"

#undef DEBUG

int resize_signal = SIGWINCH;
extern bool resize_flag;

int WINSIZE_BOTTOM = 6;
int WIDTH_MENU_CURRENT = 20;
int WIDTH_MENU_DEFAULT = 20;
int WIDTH_MENU_ENLARGE = 60;

void createWindow(WINDOW*& win, int size_y, int size_x, int pos_x, int pos_y) {
    if (win) {
        delwin(win);
    }
    win = newwin(size_y, size_x, pos_y, pos_x);
    refresh();
    box(win, 0, 0);
    wrefresh(win);
}


int main (int argc, char* argv[]) {
    std::string filename;
#ifdef DEBUG
    FileBrowser browser;
    browser.populate("build/Pb_proton_1000MeV_20mm_merged.root");
#else
    if (argc == 2) {
        if (std::filesystem::exists(argv[1])) {
            filename = argv[1];
        }
        else {
            std::cerr << "File not found" << std::endl;
            return EXIT_FAILURE;
        }
    }
    else {
        return EXIT_FAILURE;
    }
#endif

    // Init NCURSES
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    halfdelay(20);
    signal(resize_signal, [](int signum) { resize_flag = true; });

    mouseinterval(0);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);

    int sizex;
    int sizey;
    getmaxyx(stdscr, sizey, sizex);

    // Initial window setup
    WINDOW* mainwindow = nullptr;
    WINDOW* dir_window = nullptr;
    createWindow(mainwindow, sizey - WINSIZE_BOTTOM, sizex - WIDTH_MENU_CURRENT, WIDTH_MENU_CURRENT, 0);
    createWindow(dir_window, sizey - WINSIZE_BOTTOM, WIDTH_MENU_CURRENT, 0, 0);

    refresh();
    box(mainwindow, 0, 0);
    wrefresh(mainwindow);
    bool running = true;
    int nredraws = 0;

    FileBrowser browser(dir_window);
    browser.loadfile(filename.c_str());

    int last_mouse_event = 0;
    MEVENT mouse_event;

    while (running) {
        browser.printFiles(sizey, sizex, 1, 1);

        int input = getch();
        
        mvprintw(0, 0, std::to_string(nredraws++).c_str());
        if (resize_flag) {
            resize_flag = false;
            endwin();
            refresh();
            getmaxyx(stdscr, sizey, sizex);
            createWindow(mainwindow, sizey - WINSIZE_BOTTOM, sizex - WIDTH_MENU_CURRENT, WIDTH_MENU_CURRENT, 0);
            createWindow(dir_window, sizey - WINSIZE_BOTTOM, WIDTH_MENU_CURRENT, 0, 0);
        }

        if (input == 'q') {
            running = false;
        }
        else {
            browser.handleKeyPress(mainwindow, mouse_event, input);
        }
    }

    delwin(mainwindow);
    endwin();
    return EXIT_SUCCESS;
}


