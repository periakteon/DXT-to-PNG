#include "converter.h"
#include <filesystem>
#include <print>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::println("Usage: N3TexConverter <input.dxt> [output.png]");
        return 1;
    }

    const std::filesystem::path input = argv[1];
    const std::filesystem::path output = (argc >= 3)
        ? std::filesystem::path(argv[2])
        : input.parent_path() / (input.stem().string() + ".png");

    if (const auto result = Convert(input, output); !result) {
        std::println(stderr, "Error: {}", result.error());
        return 1;
    }

    return 0;
}
