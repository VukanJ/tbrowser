#include "Console.h"
#include "TVirtualTreePlayer.h"
#include <ncurses.h>
#include <stdexcept>
#include <format>

Console::Console() {
    std::string chars = " \",._<>()[]=!&|?+-*/%:@$";
    for (char c : chars) allowed_chars.insert(c);
}

bool Console::parse() {
    // Remove whitespace and unneccessary characters
    for (auto c = current_input.begin(); c != current_input.end();) {
        if (*c == ' ' || *c == '\"') { c = current_input.erase(c); }
        else { c++; }
    }

    curs_offset = 0;
    current_args = {"", "", "", TVirtualTreePlayer::kMaxEntries, 0};

    // Tokenize string by comma
    std::vector<std::string> tokens {""};
    for (auto c : current_input) {
        if (c == ',') {
            tokens.emplace_back();
        }
        else {
            tokens.back() += c;
        }
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

    return valid;
}

void Console::handleInput(int key) {
    if (valid_char(key)) {
        if (curs_offset > 0) {
            current_input.insert(current_input.size() - curs_offset, 1, static_cast<char>(key));
        }
        else {
            current_input += static_cast<char>(key);
        }
    }
    else {
        switch (key) {
            case KEY_ENTER: case 10: // Execute
                parse();
                entering_draw_command = false;
                break;
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

bool Console::valid_char(int c) {
    return isalnum(c) || allowed_chars.contains(c);
}

