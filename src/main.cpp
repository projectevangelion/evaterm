#include "Window.hpp"
#include "Config.hpp"
#include <iostream>
#include <vector>
#include <string>

void print_help(const char* prog) {
    std::cout << "EvaTerm - Custom GPU-Accelerated Tabbed Terminal Emulator\n\n"
              << "Usage: " << prog << " [options] [-e <command> [args...]]\n\n"
              << "Options:\n"
              << "  -t, --theme <name>        Theme to use (crimson_flame, catppuccin_mocha, tokyo_night, dracula, nord, gruvbox_dark)\n"
              << "  -f, --font <family>       Font family name (e.g., 'Noto Sans Mono', 'Fira Code')\n"
              << "  -s, --size <pt>           Font size in points (default: 12)\n"
              << "  -c, --config <file>       Path to custom config.ini\n"
              << "  -e, --exec <cmd...>       Execute command in initial tab\n"
              << "  -h, --help                Show this help message\n"
              << "  -v, --version             Show version information\n\n"
              << "Keyboard Shortcuts:\n"
              << "  Ctrl + Shift + T          Create new tab\n"
              << "  Ctrl + Shift + W          Close active tab\n"
              << "  Ctrl + Tab                Next tab\n"
              << "  Ctrl + Shift + Tab        Previous tab\n"
              << "  Alt + 1..9                Switch to tab 1..9\n"
              << "  Ctrl + Shift + C          Copy selection to clipboard\n"
              << "  Ctrl + Shift + V          Paste from clipboard\n"
              << "  Ctrl + Shift + R          Hot-reload configuration\n"
              << "  Ctrl + Plus / Minus / 0   Zoom in / Zoom out / Reset font size\n"
              << "  Shift + PageUp / PageDown Scroll terminal history\n";
}

int main(int argc, char* argv[]) {
    evaterm::Config config = evaterm::Config::load_default();

    std::string exec_cmd;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "EvaTerm 1.0.0 (C++20 / SDL2 / OpenGL / FreeType)\n";
            return 0;
        } else if ((arg == "-t" || arg == "--theme") && i + 1 < argc) {
            config.apply_theme_by_name(argv[++i]);
        } else if ((arg == "-f" || arg == "--font") && i + 1 < argc) {
            config.font_family = argv[++i];
        } else if ((arg == "-s" || arg == "--size") && i + 1 < argc) {
            config.font_size = std::stoi(argv[++i]);
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config = evaterm::Config::load_from_file(argv[++i]);
        } else if (arg == "-e" || arg == "--exec") {
            // Remaining arguments become the command
            std::string cmd;
            for (int j = i + 1; j < argc; ++j) {
                if (!cmd.empty()) cmd += " ";
                cmd += argv[j];
            }
            config.shell_path = cmd;
            break;
        }
    }

    std::cout << "[EvaTerm] Starting with theme: " << config.theme.name << " (" << config.theme_name << ")\n";
    evaterm::Window window(config);
    if (!window.init()) {
        std::cerr << "[EvaTerm] Failed to initialize window.\n";
        return 1;
    }

    window.run();
    return 0;
}
