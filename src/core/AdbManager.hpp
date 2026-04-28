#pragma once
#include <string>

class AdbManager {
public:
    AdbManager();
    ~AdbManager() = default;

    bool is_device_connected();
    bool dump_ui_hierarchy();
    bool pull_dump_file(const std::string& local_path);
    bool take_screenshot(const std::string& remote_path = "/sdcard/screen.png");
    bool pull_screenshot(const std::string& remote_path, const std::string& local_path);
    bool ensure_temp_directory();

    std::string get_error_message() const { return error_message_; }
    std::string get_cache_directory() const;

    static constexpr const char* REMOTE_DUMP_PATH = "/sdcard/ui_dump.xml";
    static constexpr const char* LOCAL_CACHE_DIR = "/tmp/top_window";

private:
    std::string execute_command(const std::string& cmd, bool capture_output = true);
    bool directory_exists(const std::string& path);
    bool create_directory(const std::string& path);

    std::string error_message_;
};
