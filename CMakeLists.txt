cmake_minimum_required(VERSION 3.16)
project(evaterm VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
pkg_check_modules(OPENGL REQUIRED gl)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
)

link_directories(
    ${SDL2_LIBRARY_DIRS}
    ${FREETYPE_LIBRARY_DIRS}
    ${FONTCONFIG_LIBRARY_DIRS}
    ${OPENGL_LIBRARY_DIRS}
)

set(CORE_SOURCES
    include/Cell.hpp
    src/Theme.cpp
    src/Config.cpp
    src/Pty.cpp
    src/ScreenBuffer.cpp
    src/AnsiParser.cpp
    src/Terminal.cpp
    src/Tab.cpp
    src/TabManager.cpp
    src/TabBar.cpp
    src/FontAtlas.cpp
    src/Renderer.cpp
    src/InputHandler.cpp
    src/Window.cpp
)

# Main Application Executable
add_executable(evaterm src/main.cpp ${CORE_SOURCES})

target_link_libraries(evaterm PRIVATE
    ${SDL2_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${OPENGL_LIBRARIES}
    util
    pthread
)

# Unit Tests
enable_testing()

add_executable(test_parser tests/test_parser.cpp src/AnsiParser.cpp src/ScreenBuffer.cpp src/Theme.cpp)
target_link_libraries(test_parser PRIVATE util)
add_test(NAME ParserTest COMMAND test_parser)

add_executable(test_buffer tests/test_buffer.cpp src/ScreenBuffer.cpp src/Theme.cpp)
target_link_libraries(test_buffer PRIVATE util)
add_test(NAME BufferTest COMMAND test_buffer)

# Installation targets
include(GNUInstallDirs)
install(TARGETS evaterm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY config/ DESTINATION ${CMAKE_INSTALL_DATADIR}/evaterm/config)
install(FILES terminfo/xterm-evaterm.terminfo DESTINATION ${CMAKE_INSTALL_DATADIR}/evaterm/terminfo)
