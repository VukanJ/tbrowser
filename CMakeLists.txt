cmake_minimum_required(VERSION 3.12)
set(PROGRAM browse)
project(${PROGRAM})

set(CMAKE_CXX_STANDARD 17) # ROOT needs C++17 still
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ROOT & ncurses
find_package(Curses REQUIRED)
find_package(ROOT REQUIRED)

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
        include(FetchContent)
        FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 11.1.3
        )
        FetchContent_MakeAvailable(fmt)
    endif()
endif()

include_directories(${CURSES_INCLUDE_DIR})
include(${ROOT_USE_FILE})

# Add executable
add_executable(${PROGRAM} Main.cpp Browser.cpp AxisTicks.cpp Console.cpp RootFile.cpp)

# Link against ncurses and ROOT
if(HAS_STD_FORMAT)
    target_link_libraries(${PROGRAM} ${CURSES_LIBRARIES} ${ROOT_LIBRARIES})
else()
    target_link_libraries(${PROGRAM} ${CURSES_LIBRARIES} ${ROOT_LIBRARIES} fmt::fmt)
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
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS})
