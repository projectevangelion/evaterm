#include "Pty.hpp"
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

namespace evaterm {

Pty::Pty() = default;

Pty::~Pty() {
    cleanup();
}

Pty::Pty(Pty&& other) noexcept
    : master_fd_(other.master_fd_),
      child_pid_(other.child_pid_),
      is_closed_(other.is_closed_) {
    other.master_fd_ = -1;
    other.child_pid_ = -1;
    other.is_closed_ = true;
}

Pty& Pty::operator=(Pty&& other) noexcept {
    if (this != &other) {
        cleanup();
        master_fd_ = other.master_fd_;
        child_pid_ = other.child_pid_;
        is_closed_ = other.is_closed_;
        other.master_fd_ = -1;
        other.child_pid_ = -1;
        other.is_closed_ = true;
    }
    return *this;
}

static std::string get_evaterm_terminfo_dir() {
    const char* xdg_data = std::getenv("XDG_DATA_HOME");
    if (xdg_data && xdg_data[0] != '\0') {
        return std::string(xdg_data) + "/evaterm/terminfo";
    }
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.local/share/evaterm/terminfo";
    }
    return "/tmp/evaterm/terminfo";
}

static void ensure_terminfo_installed() {
    std::string terminfo_dir = get_evaterm_terminfo_dir();
    std::string terminfo_path = terminfo_dir + "/x/xterm-evaterm";
    if (access(terminfo_path.c_str(), F_OK) == 0) {
        return; // already installed
    }

    std::string terminfo_src =
        "xterm-evaterm|EvaTerm custom GPU-accelerated tabbed terminal emulator,\n"
        "\tam, bce, hs, km, mc5i, mir, msgr, npc, xenl,\n"
        "\tcolors#256, cols#80, it#8, lines#24, pairs#32767,\n"
        "\tuse=xterm-256color,\n";

    std::string tmp_file = "/tmp/evaterm_" + std::to_string(getpid()) + ".terminfo";
    std::ofstream out(tmp_file);
    if (out.is_open()) {
        out << terminfo_src;
        out.close();
        std::string cmd = "mkdir -p " + terminfo_dir + " && tic -x -o " + terminfo_dir + " " + tmp_file + " 2>/dev/null";
        int ret = std::system(cmd.c_str());
        (void)ret;
        unlink(tmp_file.c_str());
    }
}

bool Pty::start(int rows, int cols, const std::string& custom_shell) {
    cleanup();
    ensure_terminfo_installed();

    struct winsize ws{};
    ws.ws_row = static_cast<unsigned short>(rows > 0 ? rows : 24);
    ws.ws_col = static_cast<unsigned short>(cols > 0 ? cols : 80);
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    pid_t pid = forkpty(&master_fd_, nullptr, nullptr, &ws);
    if (pid < 0) {
        perror("forkpty");
        return false;
    }

    if (pid == 0) {
        // In child process - isolate environment from Kitty or other parent terminals
        unsetenv("KITTY_WINDOW_ID");
        unsetenv("KITTY_PID");
        unsetenv("KITTY_PUBLIC_KEY");
        unsetenv("KITTY_INSTALLATION_DIR");

        std::string terminfo_dir = get_evaterm_terminfo_dir();
        setenv("TERM", "xterm-evaterm", 1);
        setenv("COLORTERM", "truecolor", 1);
        setenv("TERMINFO", terminfo_dir.c_str(), 1);
        setenv("TERMINFO_DIRS", (terminfo_dir + ":/usr/share/terminfo:/etc/terminfo").c_str(), 1);
        setenv("EVATERM_VERSION", "1.0.0", 1);

        std::string shell = custom_shell;
        if (shell.empty()) {
            const char* env_shell = std::getenv("SHELL");
            shell = (env_shell && env_shell[0] != '\0') ? env_shell : "/bin/bash";
        }

        const char* shell_name = shell.c_str();
        const char* slash = strrchr(shell_name, '/');
        std::string prog_name = slash ? (slash + 1) : shell_name;
        // Prepend '-' for login shell feel if desired, or standard
        char* const argv[] = { const_cast<char*>(shell.c_str()), nullptr };
        execvp(shell.c_str(), argv);

        // Fallback to /bin/sh
        execlp("/bin/sh", "sh", nullptr);
        _exit(127);
    }

    // In parent process
    child_pid_ = pid;
    is_closed_ = false;

    // Set master fd to non-blocking
    int flags = fcntl(master_fd_, F_GETFL, 0);
    if (flags != -1) {
        fcntl(master_fd_, F_SETFL, flags | O_NONBLOCK);
    }

    return true;
}

void Pty::resize(int rows, int cols) {
    if (master_fd_ < 0) return;
    struct winsize ws{};
    ws.ws_row = static_cast<unsigned short>(rows > 0 ? rows : 24);
    ws.ws_col = static_cast<unsigned short>(cols > 0 ? cols : 80);
    ioctl(master_fd_, TIOCSWINSZ, &ws);
}

ssize_t Pty::read_bytes(char* buffer, size_t max_length) {
    if (master_fd_ < 0 || is_closed_) return -1;
    ssize_t count = ::read(master_fd_, buffer, max_length);
    if (count < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // No data available right now
        }
        return -1; // Actual read error or EOF
    }
    return count;
}

ssize_t Pty::write_bytes(const char* data, size_t length) {
    if (master_fd_ < 0 || is_closed_) return -1;
    size_t total_written = 0;
    while (total_written < length) {
        ssize_t written = ::write(master_fd_, data + total_written, length - total_written);
        if (written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Wait briefly or break
                break;
            }
            return -1;
        }
        total_written += written;
    }
    return total_written;
}

ssize_t Pty::write_string(const std::string& str) {
    return write_bytes(str.data(), str.size());
}

bool Pty::is_alive() {
    if (child_pid_ <= 0 || is_closed_) return false;
    int status = 0;
    pid_t result = waitpid(child_pid_, &status, WNOHANG);
    if (result == 0) {
        return true; // Still running
    } else if (result == child_pid_) {
        is_closed_ = true;
        return false; // Terminated
    }
    return false;
}

void Pty::terminate() {
    cleanup();
}

void Pty::cleanup() {
    if (master_fd_ >= 0) {
        close(master_fd_);
        master_fd_ = -1;
    }
    if (child_pid_ > 0) {
        // Send SIGHUP then SIGKILL if still alive
        kill(child_pid_, SIGHUP);
        int status;
        pid_t res = waitpid(child_pid_, &status, WNOHANG);
        if (res == 0) {
            kill(child_pid_, SIGKILL);
            waitpid(child_pid_, &status, 0);
        }
        child_pid_ = -1;
    }
    is_closed_ = true;
}

std::string Pty::get_active_process_name() const {
    if (child_pid_ <= 0) return "Terminal";
    
    // Check foreground process group of the terminal
    pid_t fg_pgrp = tcgetpgrp(master_fd_);
    pid_t target_pid = (fg_pgrp > 0) ? fg_pgrp : child_pid_;

    std::string comm_path = "/proc/" + std::to_string(target_pid) + "/comm";
    std::ifstream comm_file(comm_path);
    if (comm_file.is_open()) {
        std::string name;
        std::getline(comm_file, name);
        if (!name.empty()) {
            return name;
        }
    }
    return "Terminal";
}

} // namespace evaterm
