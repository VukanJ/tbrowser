cmake_minimum_required(VERSION 3.12)
set(PROGRAM tbrowser)
project(${PROGRAM})

set(CMAKE_CXX_STANDARD 17) # ROOT needs C++17 still :(
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ROOT & ncurses
set(NCURSES_MIN_VERSION 6.5)
set(CURSES_NEED_WIDE TRUE)
set(LOCAL_NCURSES FALSE)
set(COMPILE_NCURSES FALSE)
find_package(Curses QUIET)

# Decide whether to compile ncurses
if(CURSES_FOUND)
    # Find ncurses version
    if(NCURSES_VERSION VERSION_LESS NCURSES_MIN_VERSION)
        set(LOCAL_NCURSES TRUE)
        if(NOT EXISTS "${CMAKE_BINARY_DIR}/ncurses/include/ncursesw/curses.h")
            message(WARNING "Installed ncurses ${NCURSES_VERSION} is too old to run ${PROGRAM}. It needs to be built.")
            set(COMPILE_NCURSES TRUE)
        endif()
    endif()
else()
    set(LOCAL_NCURSES TRUE)
endif()

if(LOCAL_NCURSES)
    unset(CURSES_INCLUDE_DIR CACHE)
    unset(CURSES_LIBRARIES CACHE)
    set(CURSES_INCLUDE_DIR ${CMAKE_BINARY_DIR}/ncurses/include)
    set(CURSES_LIBRARIES ${CMAKE_BINARY_DIR}/ncurses/lib/libncursesw.so)
    include_directories(BEFORE ${CMAKE_BINARY_DIR}/ncurses/include)
else()
    include_directories(${CURSES_LIBRARIES})
endif()

include(FetchContent)
if(COMPILE_NCURSES)
    FetchContent_Declare(
        ncurses
        URL https://ftp.gnu.org/pub/gnu/ncurses/ncurses-6.5.tar.gz
    )
    FetchContent_GetProperties(ncurses)
    if (NOT ncurses_POPULATED)
        FetchContent_Populate(ncurses)

        # Build ncurses
        execute_process(COMMAND ./configure --prefix=${CMAKE_BINARY_DIR}/ncurses --with-shared WORKING_DIRECTORY ${ncurses_SOURCE_DIR})
        execute_process(COMMAND make -j COMMAND make install WORKING_DIRECTORY ${ncurses_SOURCE_DIR})
    endif()
endif()

find_package(ROOT REQUIRED)
message("Getting nlohmann-json")
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        ${NLOHMANN_JSON_VERSION}
)
FetchContent_MakeAvailable(nlohmann_json)

# Check if std::format is available
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <format>
    int main() { std::format(\"{} {}\", \"Hello\", \"World\"); }
" HAS_STD_FORMAT)
if(HAS_STD_FORMAT)
    add_compile_definitions(NATIVE_FORMAT=1)
else()
    message(WARNING " Compiler does not support <format> header. Adding fmt library")
    add_compile_definitions(NATIVE_FORMAT=0)
    find_package(fmt QUIET)
    if(NOT fmt_FOUND)
        FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 11.1.3
        )
        FetchContent_MakeAvailable(fmt)
    endif()
endif()

include(${ROOT_USE_FILE})

# Add executable
add_executable(${PROGRAM} src/Main.cpp src/Browser.cpp src/AxisTicks.cpp src/Console.cpp src/RootFile.cpp)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Link against ncurses and ROOT
if(HAS_STD_FORMAT)
    target_link_libraries(${PROGRAM} PRIVATE ${CURSES_LIBRARIES} ${ROOT_LIBRARIES} nlohmann_json::nlohmann_json)
else()
    target_link_libraries(${PROGRAM} PRIVATE ${CURSES_LIBRARIES} ${ROOT_LIBRARIES} fmt::fmt nlohmann_json::nlohmann_json)
endif()

if(DEFINED USE_UNICODE)
    add_compile_definitions(USE_UNICODE=${USE_UNICODE})
else()
    # Default = Unicode
    add_compile_definitions(USE_UNICODE=1)
endif()

# COMPILER OPTIONS
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(INFO " GCC Debug build")
        target_compile_options(${PROGRAM} PRIVATE "-Wall" "-Wpedantic" "-Wextra" "-Wunused-value" "-Wunused-function" "-Wshadow" "-g" "-O0" "-std=c++20" "-Wno-cpp")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(INFO " GCC Release build")
        target_compile_options(${PROGRAM} PRIVATE "-Wall" "-Wpedantic" "-Wextra" "-Wunused-value" "-Wunused-function" "-Wshadow" "-Ofast" "-std=c++20" "-Wno-cpp")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(INFO " Clang Debug build")
        target_compile_options(${PROGRAM} PRIVATE "-g" "-O0" "-Wall" "-Wextra" "-Wpedantic" "-std=c++20")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(INFO " Clang Release build")
        target_compile_options(${PROGRAM} PRIVATE "-Wall" "-Wextra" "-Wpedantic" "-O3" "-std=c++20")
    endif()
endif()
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
