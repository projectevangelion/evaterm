#pragma once

#include <string>
#include <vector>
#include <sys/types.h>

namespace evaterm {

class Pty {
public:
    Pty();
    ~Pty();

    Pty(const Pty&) = delete;
    Pty& operator=(const Pty&) = delete;
    Pty(Pty&& other) noexcept;
    Pty& operator=(Pty&& other) noexcept;

    bool start(int rows, int cols, const std::string& custom_shell = "");
    void resize(int rows, int cols);
    ssize_t read_bytes(char* buffer, size_t max_length);
    ssize_t write_bytes(const char* data, size_t length);
    ssize_t write_string(const std::string& str);

    bool is_alive();
    void terminate();

    int get_master_fd() const { return master_fd_; }
    pid_t get_child_pid() const { return child_pid_; }
    std::string get_active_process_name() const;

private:
    int master_fd_ = -1;
    pid_t child_pid_ = -1;
    bool is_closed_ = false;

    void cleanup();
};

} // namespace evaterm
