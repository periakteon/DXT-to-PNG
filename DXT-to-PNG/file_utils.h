#pragma once
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

std::expected<std::vector<uint8_t>, std::string>
    ReadFile(const std::filesystem::path& path);
