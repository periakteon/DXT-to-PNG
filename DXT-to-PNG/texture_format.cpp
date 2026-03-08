#include "texture_format.h"
#include <cstring>
#include <format>

std::expected<TextureFormat, std::string>
FormatFromTypeData(std::span<const uint8_t, 8> typeData) {
    char typeStr[9]{};
    std::memcpy(typeStr, typeData.data(), 8);

    if (std::strstr(typeStr, "DXT1")) return TextureFormat::DXT1;
    if (std::strstr(typeStr, "DXT3")) return TextureFormat::DXT3;
    if (std::strstr(typeStr, "DXT5")) return TextureFormat::DXT5;
    if (typeData[0] == 21)            return TextureFormat::A8R8G8B8;
    if (typeData[0] == 22)            return TextureFormat::X8R8G8B8;
    if (typeData[0] == 25)            return TextureFormat::A1R5G5B5;
    if (typeData[0] == 26)            return TextureFormat::A4R4G4B4;

    return std::unexpected(std::format(
        "Unknown format bytes [{},{},{},{},{},{},{},{}]",
        typeData[0], typeData[1], typeData[2], typeData[3],
        typeData[4], typeData[5], typeData[6], typeData[7]));
}

std::string_view FormatName(TextureFormat fmt) {
    switch (fmt) {
        case TextureFormat::DXT1:     return "DXT1";
        case TextureFormat::DXT3:     return "DXT3";
        case TextureFormat::DXT5:     return "DXT5";
        case TextureFormat::A8R8G8B8: return "A8R8G8B8";
        case TextureFormat::X8R8G8B8: return "X8R8G8B8";
        case TextureFormat::A1R5G5B5: return "A1R5G5B5";
        case TextureFormat::A4R4G4B4: return "A4R4G4B4";
    }
    return "unknown";
}
