#pragma once
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>

enum class TextureFormat { DXT1, DXT3, DXT5, A8R8G8B8, X8R8G8B8, A1R5G5B5, A4R4G4B4 };

std::expected<TextureFormat, std::string> FormatFromTypeData(std::span<const uint8_t, 8> typeData);
std::string_view FormatName(TextureFormat fmt);
