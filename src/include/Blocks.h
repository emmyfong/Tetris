#pragma once

#include <array>
#include <cstdint>

enum class BlockType { I, O, T, S, Z, J, L };

using Shape = std::array<std::array<uint8_t, 4>, 4>;

struct BlockColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class Blocks {
public:
    static const Shape& getShape(BlockType type, int rotationIndex);
    static BlockColor getColor(BlockType type);
};