#ifndef CONSOLE
#define CONSOLE

#include "RtypesCore.h"
#include <tuple>
#include <vector>
#include <string>
#include <unordered_set>

class Console {
using DrawArgs = std::tuple<const char*, const char*, Option_t*, Long64_t, Long64_t>; // TTreePlayerArgs
public:
    Console();
    void handleInput(int);
    bool validChar(int);
    void cursorMove(int);
    bool parse();

    std::vector<std::string> command_buffer;
    std::string current_input;
    int curs_offset = 0;
    bool entering_draw_command = false; // Input mode

    DrawArgs current_args{"", "", "", 0, 0};

    std::string last_error;

private:
    std::unordered_set<char> allowed_chars;
};

#endif // !CONSOLE
