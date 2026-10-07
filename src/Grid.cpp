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

bool Grid::isRowFull(int row) const {
    for (int col = 0; col < Settings::kBoardWidth; ++col) {
        if (getCell(row, col) == 0) {
            return false;
        }
    }
    return true;
}

void Grid::clearRow(int row) {
    for (int col = 0; col < Settings::kBoardWidth; ++col) {
        setCell(row, col, 0);
    }
}

void Grid::moveRowsDown(int row, int dist) {
    for (int col = 0; col < Settings::kBoardWidth; ++col) {
        setCell(row + dist, col, getCell(row, col));
        setCell(row, col, 0);
    }
}

int Grid::clearFullLines() {
    int completedLines = 0;

    for (int row = Settings::kBoardHeight - 1; row >= 0; --row) {
        if (isRowFull(row)) {
            clearRow(row);
            ++completedLines;
        }
        else if (completedLines > 0) {
            moveRowsDown(row, completedLines);
        }
    }

    return completedLines;
}