#include "Block.h"

#include <algorithm>

Block::Block(BlockType type) : type(type) {
    resetPosition();
}

const Shape& Block::getCurrentShape() const {
    return Blocks::getShape(type, rotationIdx);
}

std::tuple<int, int, int, int> Block::getShapeBounds() const {
    const Shape& shape = getCurrentShape();

    int minRow = 3;
    int maxRow = 0;
    int minCol = 3;
    int maxCol = 0;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (shape[row][col] != 0) {
                minRow = std::min(minRow, row);
                maxRow = std::max(maxRow, row);
                minCol = std::min(minCol, col);
                maxCol = std::max(maxCol, col);
            }
        }
    }

    return {minRow, maxRow, minCol, maxCol};
}

void Block::rotate() {
    rotationIdx = (rotationIdx + 1) % kRotationStates;
}

void Block::undoRotate() {
    rotationIdx = (rotationIdx - 1 + kRotationStates) % kRotationStates;
}

void Block::resetPosition() {
    row = 0;
    col = 3;
    rotationIdx = 0;
}
