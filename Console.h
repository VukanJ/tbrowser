#ifndef CONSOLE_H
#define CONSOLE_H

#include "RtypesCore.h"
#include <tuple>
#include <vector>
#include <string>
#include <unordered_set>

class Console {
public:
    struct FirstDrawArg {
        FirstDrawArg(std::string ex);
        std::string expression;
        std::vector<double> limits;
        enum class LimitError {
            NoError,
            LimitOrdering,
            LimitNumber,
            No3DHists,
            InsufficientLimits
        };
        LimitError error_code = LimitError::NoError;
        bool hist2d = false;
    };
    using DrawArgs = std::tuple<FirstDrawArg, std::string, Option_t*, Long64_t, Long64_t>; // TTreePlayerArgs
    Console();
    void handleInput(int);
    bool validChar(int);
    void cursorMove(int);
    bool parse();
    void redraw(int posy, int posx);

    bool hasCommand();
    void clearCommand();
    void setError(const char* str);

    bool entering_draw_command = false; // In focus

    DrawArgs current_args{"", "", "", 0, 0};
    std::string current_input;

private:
    bool has_command = false;
    std::string last_error;
    int curs_offset = 0;
    std::unordered_set<char> allowed_chars;
    std::vector<std::string> command_buffer;
};

#endif // !CONSOLE_H
