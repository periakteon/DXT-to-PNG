#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>

void UnpackRGB565(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b);

void DecompressColorBlock(std::span<const uint8_t, 8> src,
                          std::array<std::array<uint8_t, 4>, 16>& out,
                          bool forceOpaque);

std::vector<uint8_t> DecompressDXT1(std::span<const uint8_t> src, int w, int h);
std::vector<uint8_t> DecompressDXT3(std::span<const uint8_t> src, int w, int h);
std::vector<uint8_t> DecompressDXT5(std::span<const uint8_t> src, int w, int h);
std::vector<uint8_t> DecodeA8R8G8B8(std::span<const uint8_t> src, int w, int h);
std::vector<uint8_t> DecodeX8R8G8B8(std::span<const uint8_t> src, int w, int h);
std::vector<uint8_t> DecodeA1R5G5B5(std::span<const uint8_t> src, int w, int h);
std::vector<uint8_t> DecodeA4R4G4B4(std::span<const uint8_t> src, int w, int h);
