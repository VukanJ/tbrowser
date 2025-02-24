#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cstdint>
#include <unordered_map>

#ifndef NATIVE_FORMAT
#define NATIVE_FORMAT 1
#endif
#if NATIVE_FORMAT==1
    #include <format>
    #define fmtstring(...) std::format(__VA_ARGS__)
#else
    #include <fmt/core.h>
    #define fmtstring(...) fmt::format(__VA_ARGS__)
#endif

inline bool string_contains(const std::string& str, const std::string& x) {
#if __cplusplus>=202302L
    return str.contains(x);
#else
    return str.find(x) != str.npos;
#endif
}

enum ASCII : std::uint8_t {
    LOWER_LEFT,
    LOWER_RIGHT,
    LOWER_HALF,
    LEFT_HALF,
    RIGHT_HALF,
    STAIRS_LEFT,
    STAIRS_RIGHT,
    FULL_BLOCK,
    VOID,
};

enum ASCII_code : std::uint8_t {
    C_LOWER_LEFT   = 0b1000,
    C_LOWER_RIGHT  = 0b0100,
    C_LOWER_HALF   = 0b1100,
    C_LEFT_HALF    = 0b1010,
    C_RIGHT_HALF   = 0b0101,
    C_STAIRS_LEFT  = 0b1110,
    C_STAIRS_RIGHT = 0b1101,
    C_FULL_BLOCK   = 0b1111,
    C_VOID         = 0b0000,
};

inline std::unordered_map<ASCII_code, ASCII> ascii_map = {
    {C_VOID,         VOID},
    {C_LOWER_LEFT,   LOWER_LEFT},
    {C_LOWER_RIGHT,  LOWER_RIGHT},
    {C_LOWER_HALF,   LOWER_HALF},
    {C_LEFT_HALF,    LEFT_HALF},
    {C_RIGHT_HALF,   RIGHT_HALF},
    {C_STAIRS_LEFT,  STAIRS_LEFT},
    {C_STAIRS_RIGHT, STAIRS_RIGHT},
    {C_FULL_BLOCK,   FULL_BLOCK},
};

enum color {
    blue=1, green, red, white, yellow, white_on_blue, whiteblue, grayscale_start
};

#ifndef USE_UNICODE
#define USE_UNICODE 1
#endif

#if USE_UNICODE == 1
    #define ASCII_SUP_MINUS "⁻"
    #define SYMB_FOLDER_OPEN ""
    #define SYMB_FOLDER_CLOSED ""
    #define SYMB_TTREE ""
    #define SYMB_TLEAF ""
    #define SYMB_THIST ""
    #define SYMB_TUNKNOWN "?"
#else 
    #define ASCII_SUP_MINUS "-"
    #define SYMB_FOLDER_OPEN "f"
    #define SYMB_FOLDER_CLOSED "F"
    #define SYMB_TTREE "T"
    #define SYMB_TLEAF ","
    #define SYMB_THIST "h"
    #define SYMB_TUNKNOWN "?"
#endif // USE_UNICODE

#endif // DEFINITIONS_H
