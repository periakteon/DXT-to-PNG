#pragma once
#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

enum class FileStatus { Pending, Converting, Done, Error };

struct FileEntry {
    std::filesystem::path input;
    std::filesystem::path output;
    FileStatus status = FileStatus::Pending;
    std::string error;
};

struct AppState {
    std::mutex mtx;
    std::vector<FileEntry> files;
    std::filesystem::path outputDir; // empty = same folder as each source file

    std::atomic<int> doneCount{ 0 };
    std::atomic<int> totalCount{ 0 };
    std::atomic<bool> converting{ false };
};
