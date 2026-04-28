#pragma once
#include <string>
#include <vector>

class CacheManager {
public:
    CacheManager(const std::string& cache_dir);
    ~CacheManager() = default;

    bool has_cached_file();
    std::string get_latest_cached_file();
    std::string generate_new_cache_filename();
    void invalidate_cache();
    void clear_all_cache();

    std::string get_cache_directory() const { return cache_dir_; }

private:
    std::vector<std::string> get_cached_files();
    std::string get_timestamp_string();

    std::string cache_dir_;
};
