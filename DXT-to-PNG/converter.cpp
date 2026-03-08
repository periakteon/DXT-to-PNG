#include "converter.h"
#include "decoder.h"
#include "file_utils.h"
#include "texture_format.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstdio>
#include <cstring>
#include <format>

static constexpr std::string_view kTgaSignature = "TRUEVISION-XFILE.";

std::expected<TextureHeader, std::string>
ParseHeader(std::span<const uint8_t> data, std::span<const uint8_t>& outPixels) {
    size_t offset = 0;

    auto read32 = [&]() -> std::expected<int32_t, std::string> {
        if (offset + 4 > data.size())
            return std::unexpected("File truncated reading int32");
        int32_t v{};
        std::memcpy(&v, data.data() + offset, 4);
        offset += 4;
        return v;
    };

    // TGA detection
    if (data.size() > kTgaSignature.size() + 1) {
        const auto* footer = data.data() + data.size() - kTgaSignature.size() - 1;
        if (std::memcmp(footer, kTgaSignature.data(), kTgaSignature.size()) == 0)
            return std::unexpected("TGA-wrapped textures are not supported");
    }

    // Name length + name
    auto nameLen = read32();
    if (!nameLen) return std::unexpected(nameLen.error());
    if (*nameLen < 0 || *nameLen > 200 || offset + *nameLen > data.size())
        return std::unexpected(std::format("Invalid name length {}", *nameLen));

    std::string name(reinterpret_cast<const char*>(data.data() + offset), *nameLen);
    offset += *nameLen;

    // NTF marker (3 bytes) + special byte
    if (offset + 4 > data.size())
        return std::unexpected("File truncated at NTF marker");
    offset += 3;
    const uint8_t special = data[offset++];
    if (special & 0x04)
        return std::unexpected("Encrypted textures are not supported");

    // Width + height
    auto width  = read32(); if (!width)  return std::unexpected(width.error());
    auto height = read32(); if (!height) return std::unexpected(height.error());
    if (*width <= 0 || *height <= 0 || *width > 8192 || *height > 8192)
        return std::unexpected(std::format("Invalid dimensions {}x{}", *width, *height));

    // Format type bytes (8 bytes)
    if (offset + 8 > data.size())
        return std::unexpected("File truncated at format bytes");
    auto fmt = FormatFromTypeData(std::span<const uint8_t, 8>(data.data() + offset, 8));
    if (!fmt) return std::unexpected(fmt.error());
    offset += 8;

    outPixels = data.subspan(offset);
    return TextureHeader{ std::move(name), *fmt, *width, *height };
}

std::expected<std::vector<uint8_t>, std::string>
Decode(const TextureHeader& hdr, std::span<const uint8_t> pixels) {
    switch (hdr.format) {
        case TextureFormat::DXT1:     return DecompressDXT1(pixels, hdr.width, hdr.height);
        case TextureFormat::DXT3:     return DecompressDXT3(pixels, hdr.width, hdr.height);
        case TextureFormat::DXT5:     return DecompressDXT5(pixels, hdr.width, hdr.height);
        case TextureFormat::A8R8G8B8: return DecodeA8R8G8B8(pixels, hdr.width, hdr.height);
        case TextureFormat::X8R8G8B8: return DecodeX8R8G8B8(pixels, hdr.width, hdr.height);
        case TextureFormat::A1R5G5B5: return DecodeA1R5G5B5(pixels, hdr.width, hdr.height);
        case TextureFormat::A4R4G4B4: return DecodeA4R4G4B4(pixels, hdr.width, hdr.height);
    }
    return std::unexpected("Unreachable: unknown format in Decode()");
}

std::expected<void, std::string>
Convert(const std::filesystem::path& input, const std::filesystem::path& output) {
    auto data = ReadFile(input);
    if (!data) return std::unexpected(data.error());

    std::span<const uint8_t> pixels;
    auto hdr = ParseHeader(*data, pixels);
    if (!hdr) return std::unexpected(hdr.error());

    auto rgba = Decode(*hdr, pixels);
    if (!rgba) return std::unexpected(rgba.error());

    std::printf("Name:   %s\n", hdr->name.c_str());
    std::printf("Format: %s\n", std::string(FormatName(hdr->format)).c_str());
    std::printf("Size:   %d x %d\n", hdr->width, hdr->height);

    const int ok = stbi_write_png(output.string().c_str(),
                                  hdr->width, hdr->height, 4,
                                  rgba->data(), hdr->width * 4);
    if (!ok)
        return std::unexpected("Failed to write '" + output.string() + "'");

    std::printf("Saved:  %s\n", output.string().c_str());
    return {};
}
