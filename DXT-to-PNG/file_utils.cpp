#include "file_utils.h"
#include <fstream>

std::expected<std::vector<uint8_t>, std::string>
ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return std::unexpected("Cannot open '" + path.string() + "'");

    const auto size = file.tellg();
    if (size <= 0)
        return std::unexpected("File is empty or unreadable: " + path.string());

    file.seekg(0);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        return std::unexpected("Failed to read '" + path.string() + "'");

    return buffer;
}
