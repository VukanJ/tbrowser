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

constexpr int SIZE_CMD = 6;
constexpr int SIZE_MENU = 20;


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
    createWindow(mainwindow, sizey - SIZE_CMD, sizex - SIZE_MENU, SIZE_MENU, 0);
    createWindow(dir_window, sizey - SIZE_CMD, SIZE_MENU, 0, 0);

    refresh();
    box(mainwindow, 0, 0);
    wrefresh(mainwindow);
    bool running = true;
    int nredraws = 0;

    FileBrowser browser(dir_window);
    browser.populate(filename.c_str());

    while (running) {
        browser.printFiles(sizey, sizex, 1, 1);

        int input = getch();

        // mvprintw(0, 20, std::to_string(nredraws++).c_str());
        
        if (resize_flag) {
            resize_flag = false;
            endwin();
            refresh();
            getmaxyx(stdscr, sizey, sizex);
            createWindow(mainwindow, sizey - SIZE_CMD, sizex - SIZE_MENU, SIZE_MENU, 0);
            createWindow(dir_window, sizey - SIZE_CMD, SIZE_MENU, 0, 0);
        }

        // mvprintw(30, 30, std::to_string((int)input).c_str());
        switch (input) {
            case 'q': case 27:
                running = false;
                break;
            case KEY_DOWN:
                browser.select_down();
                break;
            case KEY_UP:
                browser.select_up();
                break;
            case 'g':
                browser.goTop();
                break;
            case 'G':
                browser.goBottom();
                break;
            case '/':
                mvprintw(30, 30, "SEARCH");
                break;
            case 'p':
                browser.toggleKeyBindings();
                browser.plotHistogram(mainwindow);
                break;
            case 's':
                browser.toggleStatsBox();
                browser.plotHistogram(mainwindow);
                break;
            case 'l':
                browser.toggleLogy();
                browser.plotHistogram(mainwindow);
                break;
            case KEY_ENTER: case 10: // ENTER only works with RightShift+Enter
                browser.plotHistogram(mainwindow);
                break;
        }
    }

    delwin(mainwindow);
    endwin();
    return EXIT_SUCCESS;
}


