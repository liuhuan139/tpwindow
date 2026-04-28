#include "CacheManager.hpp"
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>

CacheManager::CacheManager(const std::string& cache_dir)
    : cache_dir_(cache_dir) {
}

std::string CacheManager::get_timestamp_string() {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm_info);
    return std::string(buffer);
}

std::string CacheManager::generate_new_cache_filename() {
    return cache_dir_ + "/ui_dump_" + get_timestamp_string() + ".xml";
}

std::vector<std::string> CacheManager::get_cached_files() {
    std::vector<std::string> files;
    DIR* dir = opendir(cache_dir_.c_str());
    if (!dir) {
        return files;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if ((name.rfind("ui_dump_", 0) == 0 && name.rfind(".xml") == name.length() - 4) ||
            (name.rfind("ui_dump_", 0) == 0 && name.rfind(".png") == name.length() - 4)) {
            files.push_back(cache_dir_ + "/" + name);
        }
    }
    closedir(dir);
    std::sort(files.rbegin(), files.rend());
    return files;
}

bool CacheManager::has_cached_file() {
    return !get_cached_files().empty();
}

std::string CacheManager::get_latest_cached_file() {
    auto files = get_cached_files();
    for (const auto& file : files) {
        if (file.rfind(".xml") == file.length() - 4) {
            return file;
        }
    }
    return "";
}

void CacheManager::invalidate_cache() {
}

void CacheManager::clear_all_cache() {
    auto files = get_cached_files();
    for (const auto& file : files) {
        unlink(file.c_str());
    }
}
