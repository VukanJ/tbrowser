#include "Console.h"
#include "TVirtualTreePlayer.h"
#include <ncurses.h>
#include <stdexcept>
#include <format>
#include "definitions.h"

Console::Console() {
    std::string chars = " \",._<>()[]=!&|?+-*/%:@$";
    for (char c : chars) allowed_chars.insert(c);
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
    current_args = {"", "", "goff", TVirtualTreePlayer::kMaxEntries, 0};

    // Tokenize string by comma
    std::vector<std::string> tokens {""};

    // interpret varexp
    if (current_input.contains(">>(")) {
        // User is specifying histogram
        // "var1>>(100, 10, 100, 1000, 0, 1)" can be the first token
        auto start_hist_spec = current_input.find(">>(");
        auto end_hist_spec = current_input.find(")", start_hist_spec);
        tokens.back() = current_input.substr(0, end_hist_spec + 1);
        current_input = current_input.substr(end_hist_spec + 1);
    }
    if (current_input.contains(":")) {
        if (std::count(current_input.begin(), current_input.end(), ':') > 1) {
            // Cannot handle 3D hists
            last_error = "Cannot plot 3D histograms :(";
            has_command = false;
            return false;
        }
        // User is specifying 2D dimensional hist
    }

    for (auto c : current_input) {
        if (c == ',') {
            tokens.emplace_back();
        }
        else {
            tokens.back() += c;
        }
    }
    for (int i = 0; auto t : tokens) {
        mvprintw(40 + i, 40, "%i %s", i+1, t.c_str());
        i++;
    }

    // Parse entries, do basic checks
    auto ntokens = tokens.size();
    bool valid = true;
    if (ntokens >= 1) { std::get<0>(current_args) = tokens[0].c_str(); }
    if (ntokens >= 2) { std::get<1>(current_args) = tokens[1].c_str(); }
    if (ntokens >= 3) { std::get<2>(current_args) = tokens[2].c_str(); }
    try {
        if (ntokens >= 4) { std::get<3>(current_args) = std::stoll(tokens[3]); }
    }
    catch (std::invalid_argument& err) {
        last_error = std::format("Argument 4 (nentries) must be Long64_t, not \"{}\"", tokens[3]);
        valid = false;
    }
    catch (std::out_of_range& err) {
        last_error = std::format("Argument 4 (nentries) out of range [0,{}]", std::numeric_limits<Long64_t>::max());
        valid = false;
    }

    try {
        if (ntokens >= 5) { std::get<4>(current_args) = std::stoll(tokens[4]); }
    }
    catch (std::invalid_argument& err) { 
        last_error = std::format("Argument 5 (firstentry) must be Long64_t, not \"{}\"", tokens[4]);
        valid = false;
    }
    catch (std::out_of_range& err) {
        last_error = std::format("Argument 5 (firstentry) out of range [0,{}]", std::numeric_limits<Long64_t>::max());
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
                            if (int delete_pos = current_input.size() - curs_offset; delete_pos >= 0) {
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
    return isalnum(c) || allowed_chars.contains(c);
}

void Console::cursorMove(int pos) {
    if (pos >= 0 && pos < current_input.size()) {
        curs_offset = current_input.size() - pos;
    }
}

void Console::redraw(int posy, int posx) {
    // CMD hint
    attron(COLOR_PAIR(red));
    mvprintw(posy, posx - 6, "Draw(");
    attroff(COLOR_PAIR(red));

    bool error_display = false;
    if (!last_error.empty()) {
        error_display = true;
        attron(COLOR_PAIR(red));
        mvprintw(posy, posx, "%s", last_error.c_str());
        attroff(COLOR_PAIR(red));
        last_error.clear();
    }
    else {
        // Command state
        if (entering_draw_command) {
            attron(COLOR_PAIR(yellow));
            attron(A_REVERSE);
            mvprintw(posy-2, posx, " INPUT ");
            attroff(A_REVERSE);
            attroff(COLOR_PAIR(yellow));
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
