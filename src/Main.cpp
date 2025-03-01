#include <clocale>
#include <cstdlib>
#include <fcntl.h>
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

int resize_fd[2]; // PIPE


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

    pipe(resize_fd);
    fcntl(resize_fd[0], F_SETFL, O_NONBLOCK);
    signal(SIGWINCH, [](int) { 
        resize_flag = true; 
        write(resize_fd[1], "R", 1); // Notify select
    });

    MEVENT mouse_event;

    while (browser.isRunning()) {
        browser.printDirectories();

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fileno(stdin), &fds);
        FD_SET(resize_fd[0], &fds);
        
        // Wait for input or signal
        select(resize_fd[0] + 1, &fds, NULL, NULL, NULL);

        if (FD_ISSET(resize_fd[0], &fds)) {
            char buf;
            read(resize_fd[0], &buf, 1);
            // Handle resize
            browser.handleResize();
        }
        if (FD_ISSET(fileno(stdin), &fds)) {
            int input = getch();
            browser.handleInputEvent(mouse_event, input);
        }

    }

    return EXIT_SUCCESS;
}
