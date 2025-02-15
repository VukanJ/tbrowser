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

extern bool resize_flag;

int main (int argc, char* argv[]) {
    // Read argument
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

    // Initial window setup
    bool running = true;
    int nredraws = 0;

    FileBrowser browser;
    browser.loadFile(filename.c_str());

    MEVENT mouse_event;

    while (running) {
        browser.printFiles(1, 1);

        int input = getch();
        
        mvprintw(0, 0, std::to_string(nredraws++).c_str());
        if (resize_flag) {
            browser.handleResize();
        }

        if (input == 'q') {
            running = false;
        }
        else {
            browser.handleInputEvent(mouse_event, input);
        }
    }

    return EXIT_SUCCESS;
}


