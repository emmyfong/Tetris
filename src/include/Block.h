#pragma once

#include <tuple>
#include "Blocks.h"

class Block {
public:
    explicit Block(BlockType type);

    const Shape& getCurrentShape() const;
    std::tuple<int, int, int, int> getShapeBounds() const;

    void rotate();
    void undoRotate();
    void resetPosition();

    BlockType type;
    int rotationIdx;
    int row;
    int col;
};