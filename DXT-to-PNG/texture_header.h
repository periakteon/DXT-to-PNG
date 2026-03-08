#pragma once
#include "texture_format.h"
#include <cstdint>
#include <string>

struct TextureHeader {
    std::string   name;
    TextureFormat format;
    int32_t       width{};
    int32_t       height{};
};
