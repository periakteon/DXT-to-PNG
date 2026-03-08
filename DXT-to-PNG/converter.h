#pragma once
#include "texture_header.h"
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

std::expected<TextureHeader, std::string>
    ParseHeader(std::span<const uint8_t> data, std::span<const uint8_t>& outPixels);

std::expected<std::vector<uint8_t>, std::string>
    Decode(const TextureHeader& hdr, std::span<const uint8_t> pixels);

std::expected<void, std::string>
    Convert(const std::filesystem::path& input, const std::filesystem::path& output);
