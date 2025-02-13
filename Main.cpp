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
extern bool resize_flag;

void createWindow(WINDOW*& win, int size_y, int size_x, int pos_x, int pos_y) {
    if (win) {
        delwin(win);
    }

    win = newwin(size_y, size_x, pos_y, pos_x);
    
    refresh();  // Ensure stdscr is updated
    box(win, 0, 0);
    wrefresh(win);
}

int main (int argc, char* argv[]) {
    setlocale(LC_ALL, "");

    initscr();
    noecho();
    cbreak();
    curs_set(0);
    halfdelay(20);
    signal(resize_signal, [](int signum) { resize_flag = true; });

    int sizex;
    int sizey;
    getmaxyx(stdscr, sizey, sizex);

    WINDOW* mainwindow;
    createWindow(mainwindow, sizey, sizex, 0, 0);
    refresh();
    box(mainwindow, 0, 0);
    wrefresh(mainwindow);

    bool running = true;
    int nredraws = 0;
    while (running) {
        char input = getch();
        mvprintw(0, 0, std::to_string(nredraws++).c_str());
        
        if (resize_flag) {
            resize_flag = false;
            endwin();
            refresh();
            getmaxyx(stdscr, sizey, sizex);
            createWindow(mainwindow, sizey, sizex, 0, 0);
        }

        switch (input) {
            case 'q':
                running = false;
                break;
        }
    }

    delwin(mainwindow);
    endwin();
    return EXIT_SUCCESS;
}
