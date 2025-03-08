#include "Console.h"
#include "TVirtualTreePlayer.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>
#include <ncurses.h>
#include <stdexcept>
#include <algorithm>
#include "definitions.h"

Console::Console() {
    const std::string chars = " \",._<>()[]=!&|?+-*/%:@$";
    for (char c : chars) { allowed_chars.insert(c); }
}

Console::~Console() {
    // Append commands to history file
    if (!historyFileName.empty() && nCommandsParsed > 0) {
        std::ofstream histFile(historyFileName, std::ios::app);
        for (std::size_t icmd = command_history.size() - nCommandsParsed; icmd < command_history.size(); ++icmd) 
        {
            histFile << command_history.at(icmd) + '\n';
        }
        histFile.close();
    }
}

Console::FirstDrawArg::FirstDrawArg(std::string ex) {
    // Unpack a specification like "var1+var2>>(0, 1, 2, 6)"
    auto beg = ex.find(">>(");
    if (beg == std::string::npos) {
        expression = ex;
    }
    else {
        expression = ex.substr(0, beg);
        std::string num;
        for (std::size_t i = beg + 3; i < ex.size(); ++i) {
            if (isdigit(ex[i]) || ex[i] == '.' || ex[i] == '-') {
                num += ex[i];
            }
            else if (!num.empty()) {
                limits.push_back(stod(num));
                num.clear();
            }
        }
    }
    if (limits.size() == 2) {
        if (limits[0] >= limits[1]) {
            error_code = LimitError::LimitOrdering;
        }
    }
    else if (limits.size() == 4) {
        if (limits[0] >= limits[1] || limits[2] >= limits[3]) {
            error_code = LimitError::LimitOrdering;
        }
    }
    else if (!limits.empty()) {
        error_code = LimitError::LimitNumber;
    }

    const int ncolon = std::count(expression.begin(), expression.end(), ':');
    if (ncolon > 0) {
        if (ncolon > 1) {
            error_code = LimitError::No3DHists;
        }
        else if (!limits.empty() && limits.size() != 4) {
            error_code = LimitError::InsufficientLimits;
        }
        else {
            hist2d = true;
        }
    }
}

bool Console::parse() {
    if (current_input.empty()) {
        has_command = false;
        return false;
    }
    // Remove whitespace and unneccessary characters
    for (auto c = current_input.begin(); c != current_input.end();) {
        if (*c == ' ' || *c == '\"') { c = current_input.erase(c); }
        else { c++; }
    }

    curs_offset = 0;
    current_args = {{""}, "", "goff", TVirtualTreePlayer::kMaxEntries, 0};

    // Tokenize string by comma
    std::vector<std::string> tokens {""};

    // interpret varexp
    int offset = 0;
    if (string_contains(current_input, ">>(")) {
        // User is specifying histogram
        // "var1>>(10, 100, 0, 1)" can be the first token
        auto start_hist_spec = current_input.find(">>(");
        auto end_hist_spec = current_input.find(')', start_hist_spec);
        if (end_hist_spec == std::string::npos) {
            last_error = "Syntax error";
            has_command = false;
            return false;
        }
        tokens.back() = current_input.substr(0, end_hist_spec + 1);
        offset = end_hist_spec;
    }

    for (std::size_t c = offset; c < current_input.size(); ++c) {
        if (current_input[c] == ',') {
            tokens.emplace_back();
        }
        else {
            tokens.back() += current_input[c];
        }
    }

    // Parse entries, do basic checks
    auto ntokens = tokens.size();
    bool valid = true;
    if (ntokens >= 1) { std::get<0>(current_args) = FirstDrawArg(tokens[0]); }
    
    switch (std::get<0>(current_args).error_code) {
        case FirstDrawArg::LimitError::NoError: break;
        case FirstDrawArg::LimitError::LimitOrdering: 
            last_error = "Invalid hist limits: min < max is required."; 
            break;
        case FirstDrawArg::LimitError::LimitNumber: 
            last_error = "Fixed binning! Specify 2 or 4 limit arguments for X and Y."; 
            break;
        case FirstDrawArg::LimitError::No3DHists:
            last_error = "Cannot draw 3D histograms :("; 
            break;
        case FirstDrawArg::LimitError::InsufficientLimits: 
            last_error = "Zero or Four limits are required to draw a 2d histogram "; 
            break;
    }
    valid = std::get<0>(current_args).error_code == FirstDrawArg::LimitError::NoError;
    if (!valid) {
        has_command = false;
        return valid;
    }

    if (ntokens >= 2) { std::get<1>(current_args) = tokens[1]; }
    if (ntokens >= 3) { std::get<2>(current_args) = tokens[2].c_str(); }
    try {
        if (ntokens >= 4) { std::get<3>(current_args) = std::stoll(tokens[3]); }
    }
    catch (std::invalid_argument& err) {
        last_error = fmtstring("Argument 4 (nentries) must be Long64_t, not \"{}\"", tokens[3]);
        valid = false;
    }
    catch (std::out_of_range& err) {
        last_error = fmtstring("Argument 4 (nentries) out of range [0,{}]", std::numeric_limits<Long64_t>::max());
        valid = false;
    }

    try {
        if (ntokens >= 5) { std::get<4>(current_args) = std::stoll(tokens[4]); }
    }
    catch (std::invalid_argument& err) { 
        last_error = fmtstring("Argument 5 (firstentry) must be Long64_t, not \"{}\"", tokens[4]);
        valid = false;
    }
    catch (std::out_of_range& err) {
        last_error = fmtstring("Argument 5 (firstentry) out of range [0,{}]", std::numeric_limits<Long64_t>::max());
        valid = false;
    }

    has_command = valid;

    if (has_command) {
        if (command_history.empty()) {
            nCommandsParsed++;
            command_history.push_back(current_input);
        }
        else if (command_history.back() != current_input) {
            // Only save new commands
            nCommandsParsed++;
            command_history.push_back(current_input);
        }
    }

    return valid;
}

void Console::setTabCompletionDict(const std::vector<RootFile::MenuItem>& dict) {
    branch_names.reserve(dict.size());
    for (const auto& [name, node] : dict) {
        branch_names.push_back(name);
    }
}

void Console::tabComplete() {
    // Get current partial branch
    if (current_input.empty()) {
        return;
    }
    if (!(isalnum(current_input.back()) || current_input.back() == '_')) {
        // Some other math symbol: ignore
        return;
    }
    std::string partial;
    for (int i = current_input.size() - 1; i >= 0; --i) {
        char c = current_input[i];
        if (isalnum(c) || c == '_') {
            partial += c;
        }
        else {
            break;
        }
    }
    // Collect potential matches
    std::vector<std::string> matches;
    if (!partial.empty()) {
        std::reverse(partial.begin(), partial.end());
        for (auto& b : branch_names) {
            if (b.starts_with(partial)) {
                matches.push_back(b);
            }
        }
    }
    // Find longest match
    if (matches.empty()) {
        return;
    }

    std::string common = matches[0];
    for (std::size_t i = 1; i < matches.size(); ++i) {
        auto mismatch_pos = std::mismatch(common.begin(), common.end(), matches[i].begin(), matches[i].end());
        common.erase(mismatch_pos.first, common.end());
        if (common.empty()) break;
    }

    // Replace
    if (!common.empty()) {
        current_input.erase(current_input.end() - partial.size(), current_input.end());
        current_input += common;
        curs_offset = 0;
    }
}

void Console::handleInput(int key) {
    if (validChar(key)) {
        if (curs_offset > 0) {
            current_input.insert(current_input.size() - curs_offset, 1, static_cast<char>(key));
        }
        else {
            current_input += static_cast<char>(key);
        }
    }
    else {
        switch (key) {
            case 27: // ESCAPE
                entering_draw_command = false; 
                break;
            case KEY_LEFT: // Move input cursor to the left
                if (curs_offset < static_cast<int>(current_input.size())) {
                    curs_offset++;
                }
                break;
            case KEY_RIGHT: // Move input cursor to the right
                if (curs_offset > 0) {
                    curs_offset--;
                }
                break;
            case KEY_UP:
                if (historyScrollback < static_cast<int>(command_history.size())) {
                    historyScrollback++;
                    if (historyScrollback == 1) {
                        // Save current input
                        current_input_stash = current_input;
                    }
                    current_input = command_history[command_history.size() - historyScrollback];
                }
                break;
            case KEY_DOWN:
                if (historyScrollback > 0) {
                    historyScrollback--;
                    if (historyScrollback == 0) {
                        // Pop from command stash
                        current_input = current_input_stash;
                    }
                    else {
                        current_input = command_history[command_history.size() - historyScrollback];
                    }
                }
                break;
            case KEY_BACKSPACE: case KEY_DC:
                {
                    if (!current_input.empty()) {
                        if (curs_offset > 0) {
                            // Delete inside string
                            if (int delete_pos = current_input.size() - curs_offset - (key == KEY_BACKSPACE ? 1 : 0); delete_pos >= 0) {
                                current_input.erase(delete_pos, 1);
                            }
                        }
                        else {
                            // Delete from back
                            current_input.pop_back(); 
                        }

                        if (key == KEY_DC) {
                            // In case of delete key, make sure cursor stays in place
                            curs_offset--;
                        }
                    }
                }
                break;
            case '\t': 
                tabComplete();
                break;
        }
    }
}

bool Console::validChar(int c) {
    return c > 0 && c < 128 && (std::isalnum(c) || allowed_chars.contains(c));
}

void Console::cursorMove(int pos) {
    if (pos >= 0 && pos < static_cast<int>(current_input.size())) {
        curs_offset = current_input.size() - pos;
    }
}

void Console::redraw(int posy, int posx) {
    // CMD hint
    attron(COLOR_PAIR(col_red));
    mvprintw(posy, posx - 6, "Draw(");
    attroff(COLOR_PAIR(col_red));

    bool error_display = false;
    if (!last_error.empty()) {
        error_display = true;
        attron(COLOR_PAIR(col_red));
        mvprintw(posy, posx, "%s", last_error.c_str());
        attroff(COLOR_PAIR(col_red));
        last_error.clear();
    }
    else {
        // Command state
        if (entering_draw_command) {
            attron(COLOR_PAIR(col_yellow) | A_REVERSE);
            mvprintw(posy-2, posx, " INPUT ");
            attroff(COLOR_PAIR(col_yellow) | A_REVERSE);
        }
        else {
            move(posy - 2, posx);
            clrtoeol();
            mvprintw(posy-2, posx, "       ");
        }
    }

    if (historyScrollback > 0) {
        mvprintw(posy-2, posx+10, "History [%i/%ld]", historyScrollback, command_history.size());
        clrtoeol();
    }
    else {
        move(posy-2, posx+10);
        clrtoeol();
    }

    // Display input
    if (!error_display) {
        if (current_input.empty() && !entering_draw_command) {
            mvprintw(posy, posx, "Press <d>");
        }
        else {
            mvprintw(posy, posx, "%s", current_input.c_str());
        }
    }

    if (entering_draw_command) {
        // Draw blinking cursor
        if (curs_offset > 0) {
            // Draw highlighted letter
            attron(A_REVERSE);
            mvprintw(posy, posx + current_input.size() - curs_offset, "%c", current_input[current_input.size() - curs_offset]);
            attroff(A_REVERSE);
        }
        else {
            // Just draw the cursor
            attron(A_BLINK);
            mvprintw(posy, posx + current_input.size(), "█");
            attroff(A_BLINK);
        }
    }
}

bool Console::hasCommand() const {
    return has_command;
}

void Console::clearCommand() {
    has_command = false;
}

void Console::loadCommandHistory(const std::string& historyFile) {
    historyFileName = historyFile;
    command_history.clear();
    if (std::filesystem::exists(historyFileName)) {
        std::ifstream histFile(historyFileName.c_str());
        if (histFile.is_open()) {
            std::string line;
            while (std::getline(histFile, line)) {
                if (!line.empty()) {
                    command_history.push_back(line);
                }
            }
            histFile.close();
        }
    }
    else {
        std::ofstream outfile(historyFileName);
    }

    if (command_history.size() > max_history_size) {
        command_history.erase(command_history.begin(),
                              command_history.begin() + command_history.size() - max_history_size);
    }
}

void Console::setError(const char* error) {
    last_error = error;
}
