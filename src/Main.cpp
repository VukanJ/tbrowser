#include <clocale>
#include <iostream>
#include <filesystem>
#include <string>
#include <csignal>

#include <ncurses.h>
#include "Browser.h"
#include "Console.h"
#include "AxisTicks.h"
#include <TError.h>

/*
🬂🬨🬂🬀🬕🬂🬓🬞🬞🬏 🬞🬭🬏🬞  🬞 🬭🬭  🬭🬭 🬞🬞🬏
 ▐  🬕🬂🬧▐🬀🬁 ▌ ▐▐🬞🬣▐🬁🬋🬏 ▐🬋🬋🬄▐🬀🬁
 🬉  🬌🬋🬅🬉   🬈🬋🬅 🬆🬁🬄🬇🬋🬅 🬁🬋🬋🬀🬉
*/


// TODO
// - [x] Open obvious tree
// - [ ] Fix menu scroll
// - [ ] Make console active tree dependent
// - [x] histogram spec in drawcall
// - [ ] y axis
// - [ ] Toggle button for menu resize
// - [ ] TH1 plotting
// - [ ] TH2 plotting
// - [ ] Tab completion
// - [ ] Histogram buffer (quick redraw)
// - [x] Make log toggle command dependent
// - [x] Help window <?>
// - [x] Console delete fix
// - [ ] Console history
// - [ ] Console horizontal scroll 
// - [ ] Search
// - [x] Obvious tree should be used for plotting
// - [x] Settings persistence

#undef DEBUG

extern bool resize_flag;

int main (int argc, char* argv[]) {
    // Silence ROOT messages including errors
    gErrorIgnoreLevel = kFatal;

    // Read argument
    std::string filename;
#ifdef DEBUG
    Console::FirstDrawArg fa("B0_M>>(100, 5000, 6000))");
    
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
    FileBrowser browser;

    try {
        browser.loadFile(filename.c_str());
    }
    catch (std::runtime_error& error) {
        endwin();
        std::cerr << error.what() << std::endl;
        return EXIT_FAILURE;
    }

    signal(SIGWINCH, [](int) { resize_flag = true; });

    MEVENT mouse_event;

    while (browser.isRunning()) {
        browser.printDirectories();
        int input = getch();
        
        if (resize_flag) {
            browser.handleResize();
        }
        browser.handleInputEvent(mouse_event, input);
    }

    return EXIT_SUCCESS;
}
