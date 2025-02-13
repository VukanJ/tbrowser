cmake_minimum_required(VERSION 3.12)
project(browse)

# Find ROOT & ncurses
find_package(Curses REQUIRED)
find_package(ROOT REQUIRED)

include_directories(${CURSES_INCLUDE_DIR})
include(${ROOT_USE_FILE})

# Add executable
add_executable(browse Main.cpp)

# Link against ncurses and ROOT
target_link_libraries(browse ${CURSES_LIBRARIES} ${ROOT_LIBRARIES})
