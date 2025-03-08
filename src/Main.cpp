#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <clocale>

#include <ncurses.h>
#include "Browser.h"
#include <TError.h>

/*
🬂🬨🬂🬀🬕🬂🬓🬞🬞🬏 🬞🬭🬏🬞  🬞 🬭🬭  🬭🬭 🬞🬞🬏 
 ▐  🬕🬂🬧▐🬀🬁 ▌ ▐▐🬞🬣▐🬁🬋🬏 ▐🬋🬋🬄▐🬀🬁 
 🬉  🬌🬋🬅🬉   🬈🬋🬅 🬆🬁🬄🬇🬋🬅 🬁🬋🬋🬀🬉   
*/

// TODO
// - [x] Open obvious tree
// - [x] Fix menu scroll
// - [x] Make console active tree dependent
// - [x] histogram spec in drawcall
// - [x] y axis
// - [x] Toggle button for menu resize
// - [ ] TH1 plotting
// - [ ] TH2 plotting
// - [x] Tab completion
// - [ ] Histogram buffer (quick redraw)
// - [x] Menu resize
// - [x] Make log toggle command dependent
// - [x] Help window <?>
// - [x] Console delete fix
// - [x] Console history
// - [x] Log y axis ticks
// - [ ] Console horizontal scroll 
// - [ ] Bring rightmost bin inside range
// - [ ] Color picker
// - [x] Fix log y range
// - [x] Search
// - [x] Obvious tree should be used for plotting
// - [x] Settings persistence

#undef DEBUG

extern bool resize_flag;

int resize_fd[2]; // PIPE

int main(int argc, char* argv[]) {
    // Silence ROOT messages including errors
    gErrorIgnoreLevel = kFatal;

    // Read argument
    std::string filename;
#ifdef DEBUG
    AxisTicks at(0, 218.900005, 5, true);
    at.setAxisPixels(55);
    std::cout << fmtstring("[{} {}]\n", at.min(), at.max());
    for (int t = 0; t < at.nticks; ++t) {
        auto tick = at.getTick(t, true);
        std::cout << fmtstring("{} {} {}", t, tick.char_position, tick.tickstr) << std::endl;
    }
    return 0;
#else
    if (argc == 1) {
        std::cout << "Usage: tbrowser <file.root>" << std::endl;
        return EXIT_SUCCESS;
    }
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
        browser.loadFile(filename);
    }
    catch (std::runtime_error& error) {
        endwin();
        std::cerr << error.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Make pipe
    if (pipe(resize_fd) == -1) {
        perror("pipe error");
        return EXIT_FAILURE;
    }
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
            char buf = 0;
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
