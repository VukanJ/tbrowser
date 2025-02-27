#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cstdint>
#include <array>
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

inline constexpr std::array<const char[4], 8>  ascii_2x2 { "▖", "▗", "▄", "▌", "▐", "▙", "▟", "█" };
inline constexpr std::array<const char[5], 16> ascii_3x2 { "🬏", "🬞", "🬭", "🬱", "🬵", "🬹", "🬓", "🬦", "▌", "▐", "🬲", "🬷", "🬺", "🬻", "█"};

enum BLOCKS_2x2 : std::uint8_t {
    a_LOWER_LEFT,
    a_LOWER_RIGHT,
    a_LOWER_HALF,
    a_LEFT_HALF,
    a_RIGHT_HALF,
    a_STAIRS_LEFT,
    a_STAIRS_RIGHT,
    a_FULL_BLOCK,
    a_VOID,
};

enum BLOCKS_code_2x2 : std::uint8_t {
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

enum BLOCKS_3x2 : std::uint8_t {
    A_LOWER_LEFT,   // "🬏",
    A_LOWER_RIGHT,  // "🬞",
    A_LOWER_SLAB,   // "🬭",
    A_STAIRS_L,     // "🬱",
    A_STAIRS_R,     // "🬵",
    A_SMALL_BLOCK,  // "🬹",
    A_LEFT_PILLAR,  // "🬓",
    A_RIGHT_PILLAR, // "🬦",
    A_LEFT_WALL,    // "▌",
    A_RIGHT_WALL,   // "▐",
    A_STEEP_L,      // "🬲",
    A_STEEP_R,      // "🬷",
    A_BSTAIR_L,     // "🬺",
    A_BSTAIR_R,     // "🬻",
    A_FULL_BLOCK,   // "█"
    A_VOID          
};
enum BLOCKS_code_3x2 : std::uint8_t {
    EC_LOWER_LEFT   = 0b100000, // "🬏", 
    EC_LOWER_RIGHT  = 0b010000, // "🬞", 
    EC_LOWER_SLAB   = 0b110000, // "🬭", 
    EC_STAIRS_L     = 0b111000, // "🬱", 
    EC_STAIRS_R     = 0b110100, // "🬵", 
    EC_SMALL_BLOCK  = 0b111100, // "🬹", 
    EC_LEFT_PILLAR  = 0b101000, // "🬓", 
    EC_RIGHT_PILLAR = 0b010100, // "🬦", 
    EC_LEFT_WALL    = 0b101010, // "▌", 
    EC_RIGHT_WALL   = 0b010101, // "▐", 
    EC_STEEP_L      = 0b111010, // "🬲", 
    EC_STEEP_R      = 0b110101, // "🬷", 
    EC_BSTAIR_L     = 0b111110, // "🬺", 
    EC_BSTAIR_R     = 0b111101, // "🬻", 
    EC_FULL_BLOCK   = 0b111111, // "█"
    EC_VOID         = 0b000000,
};

inline std::unordered_map<BLOCKS_code_2x2, BLOCKS_2x2> ascii_map_2x2 = {
    {C_VOID,         a_VOID},
    {C_LOWER_LEFT,   a_LOWER_LEFT},
    {C_LOWER_RIGHT,  a_LOWER_RIGHT},
    {C_LOWER_HALF,   a_LOWER_HALF},
    {C_LEFT_HALF,    a_LEFT_HALF},
    {C_RIGHT_HALF,   a_RIGHT_HALF},
    {C_STAIRS_LEFT,  a_STAIRS_LEFT},
    {C_STAIRS_RIGHT, a_STAIRS_RIGHT},
    {C_FULL_BLOCK,   a_FULL_BLOCK},
};

inline std::unordered_map<std::uint8_t, std::uint8_t> ascii_map_3x2 = {
    {EC_VOID,         A_VOID},
    {EC_LOWER_LEFT,   A_LOWER_LEFT},
    {EC_LOWER_RIGHT,  A_LOWER_RIGHT},
    {EC_LOWER_SLAB,   A_LOWER_SLAB},
    {EC_STAIRS_L,     A_STAIRS_L},
    {EC_STAIRS_R,     A_STAIRS_R},
    {EC_SMALL_BLOCK,  A_SMALL_BLOCK},
    {EC_LEFT_PILLAR,  A_LEFT_PILLAR},
    {EC_RIGHT_PILLAR, A_RIGHT_PILLAR},
    {EC_LEFT_WALL,    A_LEFT_WALL},
    {EC_RIGHT_WALL,   A_RIGHT_WALL},
    {EC_STEEP_L,      A_STEEP_L},
    {EC_STEEP_R,      A_STEEP_R},
    {EC_BSTAIR_L,     A_BSTAIR_L},
    {EC_BSTAIR_R,     A_BSTAIR_R},
    {EC_FULL_BLOCK,   A_FULL_BLOCK},
};

enum color {
    blue=1, green, red, white, yellow, white_on_blue, whiteblue,
    grayscale_start, grayscale_end=grayscale_start+26
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
