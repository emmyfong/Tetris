#pragma once

#include <array>
#include <cassert>
#include <cstdint>

#include "Settings.h"

class Grid {
public:
    bool isInBounds(int row, int col) const;
    uint8_t getCell(int row, int col) const;
    void setCell(int row, int col, uint8_t val);

    bool isRowFull(int row) const;
    void clearRow(int row);
    void moveRowsDown(int row, int dist);
    int clearFullLines();

private:
    int index(int row, int col) const;

    std::array<uint8_t, Settings::kBoardWidth * Settings::kBoardHeight> cells{};

};