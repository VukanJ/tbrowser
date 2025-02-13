#include <cstdint>
#include <iostream>
#include <filesystem>
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

int main (int argc, char* argv[]) {
    setlocale(LC_ALL, "");

    signal(resize_signal, [](int signum) { resize_flag = true; });

#ifdef DEBUG
    FileBrowser f("Pb_gamma_100000MeV_10mm_merged.root");
    f.populate();
#else
    if (argc != 2) {
        std::cerr << "Expected single argument <file>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string filename = argv[1];
    if (std::filesystem::exists(filename)) {
        Viewer viewer;
        viewer.Main();
    }
    else {
        std::cerr << "File not found: " << filename << std::endl;
        return EXIT_FAILURE;
    }
#endif

    return EXIT_SUCCESS;
}
