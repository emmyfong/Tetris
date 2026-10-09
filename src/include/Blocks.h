#pragma once

#include <array>
#include <cstdint>
#include "Settings.h"

enum class BlockType { I, O, T, S, Z, J, L };

using Shape = std::array<std::array<uint8_t, 4>, 4>;
constexpr int kRotationStates = 4;

class Blocks {
public:
    static const Shape& getShape(BlockType type, int rotationIndex);
    static Settings::Color getColor(BlockType type);
};