# EvaTerm

A modern, fast, GPU-accelerated tabbed terminal emulator written in Modern C++ (C++20) using SDL2, OpenGL, and FreeType.

![Theme](https://img.shields.io/badge/Default%20Theme-Crimson%20Flame-cf2824)
![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

---

## Features

- **Multi-Tab Session Architecture**:
  - Independent concurrent PTY processes running in the background for each tab.
  - Interactive top Tab Bar with active indicators, close buttons `(x)`, and add button `(+)`.
  - Full keyboard shortcuts (`Ctrl+Shift+T`, `Ctrl+Shift+W`, `Ctrl+Tab`, `Alt+1..9`).
  - Automatic tab titles via OSC escape sequences and process name introspection.
- **Customizable Configuration System (`evaterm.conf`)**:
  - Full config file format supporting `key value`, inline colors, and `include` directives.
  - **Live Inotify Hot-Reloading**: Changes to `~/.config/evaterm/evaterm.conf` are automatically applied the moment you save the file in your editor.
  - Pre-configured to match your **Crimson Flame** Kitty theme by default.
  - Built-in theme presets: `crimson_flame`, `catppuccin_mocha`, `tokyo_night`, `dracula`, `nord`, `gruvbox_dark`.
  - Full 16 ANSI colors, 256 colors, and 24-bit TrueColor RGB support.
- **Kitty Graphics Protocol & Image Rendering**:
  - Full support for the **Kitty Graphics Protocol** (`\033_G...;\033\`).
  - Supports PNG, JPEG, GIF, BMP, and 24/32-bit raw RGB/RGBA image streams directly in your terminal.
  - Works with CLI tools like `kitten icat`, `fastfetch`, `yazi`, `viu`, `chafa`, and `ranger`.
  - Images scroll naturally with the terminal grid and scrollback history.
- **High-Performance Rendering**:
  - GPU-accelerated OpenGL text quad rendering with FreeType font engine and 32-bit RGBA dynamic glyph atlas.
  - Crisp, anti-aliased font rendering with integer pixel coordinate snapping.
  - Configurable font size with live zooming (`Ctrl++`, `Ctrl+-`, `Ctrl+0`).
- **Complete VT100 / ANSI Terminal Emulation**:
  - Primary and Alternate Screen Buffers (`neovim`, `vim`, `htop`, `tmux`, `cmatrix`, `less`).
  - Isolated cursor states preserving your exact shell position when closing full-screen TUIs.
  - Scrollback history buffer (up to 10,000+ lines).
  - Text-only selection highlighting with word/line double/triple-click and clipboard copy/paste (`Ctrl+Shift+C`, `Ctrl+Shift+V`, middle-click).

---

## Keybindings & Shortcuts

| Shortcut | Action |
| :--- | :--- |
| `Ctrl + Shift + T` | Open new tab |
| `Ctrl + Shift + W` | Close active tab |
| `Ctrl + Tab` | Switch to next tab |
| `Ctrl + Shift + Tab` | Switch to previous tab |
| `Alt + 1` .. `Alt + 9` | Jump directly to tab 1 through 9 |
| `Ctrl + Shift + C` | Copy selected text to clipboard |
| `Ctrl + Shift + V` | Paste text from clipboard |
| `Ctrl + Shift + R` | Manual hot-reload `~/.config/evaterm/evaterm.conf` |
| `Ctrl + =` / `Ctrl + +` | Zoom in font size |
| `Ctrl + -` | Zoom out font size |
| `Ctrl + 0` | Reset font size to default |
| `Shift + PageUp` / `PageDown` | Scroll terminal history up/down |

---

## Configuration (~/.config/evaterm/evaterm.conf)

EvaTerm uses a clean `key value` configuration file format:

```conf
# --- Theme Preset ---
# Options: crimson_flame, catppuccin_mocha, tokyo_night, dracula, nord, gruvbox_dark
theme crimson_flame

# --- Font Configuration ---
font_family monospace
font_size   11.0

# --- Window & Appearance ---
background_opacity 1.0
padding_x          8
padding_y          6

# --- Tab Bar ---
show_tab_bar_single_tab yes
tab_bar_height          30

# --- Shell & Terminal Behavior ---
shell               
scrollback_lines    10000
cursor_shape        block
cursor_blink        yes
cursor_blink_interval 500

# --- Color Scheme Overrides (Optional) ---
background           #25090a
foreground           #ebdada
cursor               #e32a10
cursor_text_color    #25090a
selection_background #cf2824
selection_foreground #ffffff

active_tab_background   #cf2824
active_tab_foreground   #ffffff
inactive_tab_background #360e10
inactive_tab_foreground #b58487
tab_bar_background      #1a0506

# 16 Terminal ANSI Colors
color0  #330d0f
color1  #cf2824
color2  #e0682b
color3  #e6ab3c
color4  #ff7373
color5  #bd5486
color6  #ff9f68
color7  #cfbebe
color8  #6e282c
color9  #e32a10
color10 #ff843d
color11 #ffc85a
color12 #ffa19c
color13 #de6ea4
color14 #ffbe94
color15 #faecec
```

---

## Building & Running

```bash
# Build
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Run
./build/evaterm
```
