cmake_minimum_required(VERSION 3.16)
project(CheStrat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SFML 2
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Engine library sources
set(ENGINE_SOURCES
    src/core/bitboard.cpp
    src/core/board.cpp
    src/core/movegen.cpp
    src/engine/engine.cpp
    src/eval/evaluation.cpp
    src/search/search.cpp
    src/search/ttable.cpp
)

add_library(chestrat_engine STATIC ${ENGINE_SOURCES})
target_include_directories(chestrat_engine PUBLIC ${CMAKE_SOURCE_DIR}/src)

# GUI executable
set(GUI_SOURCES
    gui/main.cpp
    gui/gui.cpp
    gui/renderer.cpp
)

add_executable(CheStrat ${GUI_SOURCES})
target_link_libraries(CheStrat PRIVATE chestrat_engine sfml-graphics sfml-window sfml-system)

# Copy assets to build directory
add_custom_command(TARGET CheStrat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CheStrat>/assets
)
