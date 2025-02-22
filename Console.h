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
    void redraw(int posy, int posx);

    bool entering_draw_command = false; // In focus

    DrawArgs current_args{"", "", "", 0, 0};
    std::string current_input;

private:
    std::string last_error;
    int curs_offset = 0;
    std::unordered_set<char> allowed_chars;
    std::vector<std::string> command_buffer;
};

#endif // !CONSOLE
