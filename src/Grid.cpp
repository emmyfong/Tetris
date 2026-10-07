#include "Grid.h"

int Grid::index(int row, int col) const {
    return row * Settings::kBoardWidth + col;
}

bool Grid::isInBounds(int row, int col) const {
    return row >= 0 && row < Settings::kBoardHeight && col >= 0 && col < Settings::kBoardWidth;
}

uint8_t Grid::getCell(int row, int col) const {
    assert(isInBounds(row, col));
    return cells[index(row, col)];
}

void Grid::setCell(int row, int col, uint8_t value) {
    assert(isInBounds(row, col));
    cells[index(row, col)] = value;
}