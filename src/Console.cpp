#include "Console.h"
#include "TVirtualTreePlayer.h"
#include <cmath>
#include <ncurses.h>
#include <stdexcept>
#include <stack>
#include <iostream>
#include <algorithm>
#include "definitions.h"

Console::Console() {
    std::string chars = " \",._<>()[]=!&|?+-*/%:@$";
    for (char c : chars) allowed_chars.insert(c);
}

Console::FirstDrawArg::FirstDrawArg(std::string ex) {
    // Unpack a specification like "var1+var2>>(0, 1, 2, 6)"
    auto beg = ex.find(">>(");
    if (beg == ex.npos) {
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
    else if (limits.size() != 0) {
        error_code = LimitError::LimitNumber;
    }

    int ncolon = std::count(expression.begin(), expression.end(), ':');
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
        // "var1>>(100, 10, 100, 1000, 0, 1)" can be the first token
        auto start_hist_spec = current_input.find(">>(");
        auto end_hist_spec = current_input.find(")", start_hist_spec);
        if (end_hist_spec == current_input.npos) {
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
    for (int i = 0; auto t : tokens) {
        mvprintw(40 + i, 40, "%i %s", i+1, t.c_str());
        i++;
    }

    // Parse entries, do basic checks
    auto ntokens = tokens.size();
    bool valid = true;
    if (ntokens >= 1) { std::get<0>(current_args) = FirstDrawArg(tokens[0].c_str()); }
    
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

    if (ntokens >= 2) { std::get<1>(current_args) = tokens[1].c_str(); }
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

    return valid;
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
                if (curs_offset < current_input.size()) {
                    curs_offset++;
                }
                break;
            case KEY_RIGHT: // Move input cursor to the right
                if (curs_offset > 0) {
                    curs_offset--;
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
                current_input += "TAB_WIP";
                break;
        }
    }
}

bool Console::validChar(int c) {
    return c > 0 && c < 128 && (std::isalnum(c) || allowed_chars.contains(c));
}

void Console::cursorMove(int pos) {
    if (pos >= 0 && pos < current_input.size()) {
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
            attron(COLOR_PAIR(col_yellow));
            attron(A_REVERSE);
            mvprintw(posy-2, posx, " INPUT ");
            attroff(A_REVERSE);
            attroff(COLOR_PAIR(col_yellow));
        }
        else {
            mvprintw(posy-2, posx, "       ");
        }
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
        attron(A_REVERSE);
        if (curs_offset > 0) {
            // Draw highlighted letter
            mvprintw(posy, posx + current_input.size() - curs_offset, "%c", current_input[current_input.size() - curs_offset]);
        }
        else {
            // Just draw the cursor
            attron(A_BLINK);
            mvprintw(posy, posx + current_input.size(), "█");
            attroff(A_BLINK);
        }
        attroff(A_REVERSE);
    }
}

bool Console::hasCommand() {
    return has_command;
}

void Console::clearCommand() {
    has_command = false;
}

void Console::setError(const char* error) {
    last_error = error;
}
