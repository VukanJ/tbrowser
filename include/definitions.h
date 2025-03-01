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
inline constexpr std::array<const char[5], 24> ascii_4x2 { 
    "⡀", "⢀", "⡄", "⢠", "⡆", "⢰", "⡇", "⢸", 
    "⣀", "⣤", "⣶", "⣿",
    "⣄", "⣠", "⣆", "⣰", "⣇", "⣸", "⣦", "⣴", "⣧", "⣼", "⣷", "⣾"
};

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
    a2_LOWER_LEFT,   // "🬏",
    a2_LOWER_RIGHT,  // "🬞",
    a2_LOWER_SLAB,   // "🬭",
    a2_STAIRS_L,     // "🬱",
    a2_STAIRS_R,     // "🬵",
    a2_SMALL_BLOCK,  // "🬹",
    a2_LEFT_PILLAR,  // "🬓",
    a2_RIGHT_PILLAR, // "🬦",
    a2_LEFT_WALL,    // "▌",
    a2_RIGHT_WALL,   // "▐",
    a2_STEEP_L,      // "🬲",
    a2_STEEP_R,      // "🬷",
    a2_BSTAIR_L,     // "🬺",
    a2_BSTAIR_R,     // "🬻",
    a2_FULL_BLOCK,   // "█"
    a2_VOID          
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

enum BLOCKS_4x2 : std::uint8_t {
    B_L1,  // "⡀", 
    B_R1,  // "⢀", 
    B_L2,  // "⡄", 
    B_R2,  // "⢠", 
    B_L3,  // "⡆", 
    B_R3,  // "⢰", 
    B_L4,  // "⡇", 
    B_R4,  // "⢸", 
    B_1,   // "⣀", 
    B_2,   // "⣤", 
    B_3,   // "⣶", 
    B_4,   // "⣿", 
    B_LS1, // "⣄", 
    B_RS1, // "⣠", 
    B_LS2, // "⣆", 
    B_RS2, // "⣰", 
    B_LS3, // "⣇", 
    B_RS3, // "⣸", 
    B_LS4, // "⣦", 
    B_RS4, // "⣴", 
    B_LS5, // "⣧", 
    B_RS5, // "⣼", 
    B_LS6, // "⣷", 
    B_RS6, // "⣾"
    B_VOID
};

enum BLOCKS_code_4x2 : std::uint8_t {
    BC_L1  = 0b10000000, // "⡀", 
    BC_R1  = 0b01000000, // "⢀", 
    BC_L2  = 0b10100000, // "⡄", 
    BC_R2  = 0b01010000, // "⢠", 
    BC_L3  = 0b10101000, // "⡆", 
    BC_R3  = 0b01010100, // "⢰", 
    BC_L4  = 0b10101010, // "⡇", 
    BC_R4  = 0b01010101, // "⢸", 
    BC_1   = 0b11000000, // "⣀", 
    BC_2   = 0b11110000, // "⣤", 
    BC_3   = 0b11111100, // "⣶", 
    BC_4   = 0b11111111, // "⣿", 
    BC_LS1 = 0b11100000, // "⣄", 
    BC_RS1 = 0b11010000, // "⣠", 
    BC_LS2 = 0b11101000, // "⣆", 
    BC_RS2 = 0b11010100, // "⣰", 
    BC_LS3 = 0b11101010, // "⣇", 
    BC_RS3 = 0b11010101, // "⣸", 
    BC_LS4 = 0b11111000, // "⣦", 
    BC_RS4 = 0b11110100, // "⣴", 
    BC_LS5 = 0b11111010, // "⣧", 
    BC_RS5 = 0b11110101, // "⣼", 
    BC_LS6 = 0b11111110, // "⣷", 
    BC_RS6 = 0b11111101, // "⣾"
    BC_VOID = 0
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
    {EC_VOID,         a2_VOID},
    {EC_LOWER_LEFT,   a2_LOWER_LEFT},
    {EC_LOWER_RIGHT,  a2_LOWER_RIGHT},
    {EC_LOWER_SLAB,   a2_LOWER_SLAB},
    {EC_STAIRS_L,     a2_STAIRS_L},
    {EC_STAIRS_R,     a2_STAIRS_R},
    {EC_SMALL_BLOCK,  a2_SMALL_BLOCK},
    {EC_LEFT_PILLAR,  a2_LEFT_PILLAR},
    {EC_RIGHT_PILLAR, a2_RIGHT_PILLAR},
    {EC_LEFT_WALL,    a2_LEFT_WALL},
    {EC_RIGHT_WALL,   a2_RIGHT_WALL},
    {EC_STEEP_L,      a2_STEEP_L},
    {EC_STEEP_R,      a2_STEEP_R},
    {EC_BSTAIR_L,     a2_BSTAIR_L},
    {EC_BSTAIR_R,     a2_BSTAIR_R},
    {EC_FULL_BLOCK,   a2_FULL_BLOCK},
};

inline std::unordered_map<std::uint8_t, std::uint8_t> ascii_map_4x2 = {
    {BC_L1,   B_L1},
    {BC_R1,   B_R1},
    {BC_L2,   B_L2},
    {BC_R2,   B_R2},
    {BC_L3,   B_L3},
    {BC_R3,   B_R3},
    {BC_L4,   B_L4},
    {BC_R4,   B_R4},
    {BC_1,    B_1,},
    {BC_2,    B_2,},
    {BC_3,    B_3,},
    {BC_4,    B_4,},
    {BC_LS1,  B_LS1},
    {BC_RS1,  B_RS1},
    {BC_LS2,  B_LS2},
    {BC_RS2,  B_RS2},
    {BC_LS3,  B_LS3},
    {BC_RS3,  B_RS3},
    {BC_LS4,  B_LS4},
    {BC_RS4,  B_RS4},
    {BC_LS5,  B_LS5},
    {BC_RS5,  B_RS5},
    {BC_LS6,  B_LS6},
    {BC_RS6,  B_RS6},
    {BC_VOID, B_VOID}
};

enum TermColor {
    col_blue=1, col_green, col_red, col_white, col_yellow, col_win_bkg, col_whiteblue,
    col_start_palette, col_end_palette = col_start_palette + (231-17), col_grayscale_start, col_grayscale_end=col_grayscale_start+26
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
