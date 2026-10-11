#include "Renderer.h"

#include <array>
#include <cstdint>
#include <iostream>

static char typeIdToChar(uint8_t typeId) {
    switch (static_cast<BlockType>(typeId - 1)) {
        case BlockType::I: return 'I';
        case BlockType::O: return 'O';
        case BlockType::T: return 'T';
        case BlockType::S: return 'S';
        case BlockType::Z: return 'Z';
        case BlockType::J: return 'J';
        case BlockType::L: return 'L';
    }
    return '?';
}

void Renderer::draw(const Engine& engine) const {
    const Grid& grid = engine.getGrid();
    const Block& activeBlock = engine.getCurrentBlock();

    std::array<std::array<uint8_t, Settings::kBoardWidth>, Settings::kBoardHeight> display{};

    for (int row = 0; row < Settings::kBoardHeight; ++row) {
        for (int col = 0; col < Settings::kBoardWidth; ++col) {
            display[row][col] = grid.getCell(row, col);
        }
    }

    const Shape& shape = activeBlock.getCurrentShape();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (shape[i][j] != 0) {
                int globalRow = activeBlock.row + i;
                int globalCol = activeBlock.col + j;
                if (grid.isInBounds(globalRow, globalCol)) {
                    display[globalRow][globalCol] = static_cast<uint8_t>(activeBlock.type) + 1;
                }
            }
        }
    }

    for (int row = 0; row < Settings::kBoardHeight; ++row) {
        for (int col = 0; col < Settings::kBoardWidth; ++col) {
            uint8_t cell = display[row][col];
            std::cout << (cell == 0 ? '.' : typeIdToChar(cell));
        }
        std::cout << '\n';
    }

    std::cout << "Score: " << engine.getScore() << '\n';
    if (engine.isGameOver()) {
        std::cout << "GAME OVER\n";
    }
}
