#ifndef CONSOLE_H
#define CONSOLE_H

#include "RtypesCore.h"
#include <tuple>
#include <vector>
#include <string>
#include <unordered_set>
#include "RootFile.h"

inline constexpr size_t max_history_size = 1000;

class Console {
public:
    Console();
    ~Console();

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

    void setTabCompletionDict(const std::vector<RootFile::MenuItem>&);
    void handleInput(int);
    bool validChar(int);
    void cursorMove(int);
    bool parse();
    void redraw(int posy, int posx);

    bool hasCommand() const;
    void clearCommand();
    void loadCommandHistory(const std::string& historyFile);
    void setError(const char* error);

    bool entering_draw_command = false; // In focus

    DrawArgs current_args{"", "", "", 0, 0};
    std::string current_input;

private:
    void tabComplete();

    std::string historyFileName;
    bool has_command = false;
    std::string last_error;
    std::unordered_set<char> allowed_chars;
    std::vector<std::string> command_history;
    std::vector<std::string> branch_names;
    int curs_offset = 0;
    int nCommandsParsed = 0;

    std::string current_input_stash;
    int historyScrollback = 0;
};

#endif // !CONSOLE_H
