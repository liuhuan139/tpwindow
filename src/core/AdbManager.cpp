#include "AdbManager.hpp"
#include <cstdio>
#include <memory>
#include <array>
#include <sys/stat.h>

AdbManager::AdbManager() {
}

std::string AdbManager::execute_command(const std::string& cmd, bool capture_output) {
    std::string result;
    std::string full_cmd = cmd;
    if (!capture_output) {
        full_cmd += " > /dev/null 2>&1";
    }
    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe) {
        error_message_ = "Failed to execute command: " + cmd;
        return "";
    }
    if (capture_output) {
        std::array<char, 256> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
    }
    int status = pclose(pipe);
    if (status != 0) {
        error_message_ = "Command failed with status " + std::to_string(status) + ": " + cmd;
        return "";
    }
    return result;
}

bool AdbManager::is_device_connected() {
    error_message_.clear();
    std::string output = execute_command("adb devices");
    if (output.empty()) {
        return false;
    }
    int device_count = 0;
    size_t pos = 0;
    while ((pos = output.find("device\n", pos)) != std::string::npos) {
        if (output.substr(pos - 10, 5) != "List ") {
            device_count++;
        }
        pos += 7;
    }
    if (device_count == 0) {
        error_message_ = "No Android device connected. Please connect a device with USB debugging enabled.";
    }
    return device_count > 0;
}

bool AdbManager::dump_ui_hierarchy() {
    error_message_.clear();
    std::string cmd = "adb shell uiautomator dump " + std::string(REMOTE_DUMP_PATH);
    std::string output = execute_command(cmd);
    if (output.empty() && !error_message_.empty()) {
        return false;
    }
    if (output.find("ERROR") != std::string::npos || output.find("error") != std::string::npos) {
        error_message_ = "uiautomator dump failed: " + output;
        return false;
    }
    return true;
}

bool AdbManager::pull_dump_file(const std::string& local_path) {
    error_message_.clear();
    std::string cmd = "adb pull " + std::string(REMOTE_DUMP_PATH) + " " + local_path;
    std::string output = execute_command(cmd);
    if (output.empty() && !error_message_.empty()) {
        return false;
    }
    return true;
}

bool AdbManager::take_screenshot(const std::string& remote_path) {
    error_message_.clear();
    std::string cmd = "adb shell screencap -p " + remote_path;
    std::string output = execute_command(cmd);
    if (output.empty() && !error_message_.empty()) {
        return false;
    }
    return true;
}

bool AdbManager::pull_screenshot(const std::string& remote_path, const std::string& local_path) {
    error_message_.clear();
    std::string cmd = "adb pull " + remote_path + " " + local_path;
    std::string output = execute_command(cmd);
    if (output.empty() && !error_message_.empty()) {
        return false;
    }
    return true;
}

bool AdbManager::directory_exists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

bool AdbManager::create_directory(const std::string& path) {
    if (mkdir(path.c_str(), 0755) != 0) {
        error_message_ = "Failed to create directory: " + path;
        return false;
    }
    return true;
}

bool AdbManager::ensure_temp_directory() {
    error_message_.clear();
    std::string dir = get_cache_directory();
    if (!directory_exists(dir)) {
        return create_directory(dir);
    }
    return true;
}

std::string AdbManager::get_cache_directory() const {
    return std::string(LOCAL_CACHE_DIR);
}
