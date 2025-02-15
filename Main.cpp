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
    browser.populate(filename.c_str());

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

        switch (input) {
            case 'q': case 27:
                running = false;
                break;
            case KEY_MOUSE:
                if (getmouse(&mouse_event) == OK) {
                    if (mouse_event.bstate == BUTTON1_PRESSED) {
                        browser.handleMouseClick(mouse_event.y, mouse_event.x);
                    }
                    else if (mouse_event.bstate == BUTTON4_PRESSED) {
                        // Scroll down once
                        browser.select_up();
                    }
                    else if (mouse_event.bstate == BUTTON5_PRESSED) {
                        // Scroll up once
                        browser.select_down();
                    }
                }
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
            case 9: // Tab
                if (WIDTH_MENU_CURRENT == WIDTH_MENU_DEFAULT) {
                    WIDTH_MENU_CURRENT = WIDTH_MENU_ENLARGE;
                }
                else if (WIDTH_MENU_CURRENT == WIDTH_MENU_ENLARGE) {
                    WIDTH_MENU_CURRENT = WIDTH_MENU_DEFAULT;
                }
                browser.plotHistogram(mainwindow);
                resize_flag = true;
                break;
        }
    }

    delwin(mainwindow);
    endwin();
    return EXIT_SUCCESS;
}


