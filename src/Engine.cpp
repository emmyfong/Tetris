#include "Engine.h"

#include <algorithm>
#include <utility>

Engine::Engine(unsigned int seed)
    : rng(seed),
      currentBlock(BlockType::I),
      nextBlock(BlockType::I),
      canHold(true),
      totalLinesCleared(0),
      level(1),
      score(0),
      gameOver(false) {
    nextBlock = getRandomBlock();
    currentBlock = getRandomBlock();
}

Block Engine::getRandomBlock() {
    if (bag.empty()) {
        bag = { BlockType::I, BlockType::O, BlockType::T, BlockType::S, BlockType::Z, BlockType::J, BlockType::L };
        std::shuffle(bag.begin(), bag.end(), rng);
    }

    BlockType type = bag.back();
    bag.pop_back();

    return Block(type);
}

void Engine::spawnBlock() {
    currentBlock = nextBlock;
    nextBlock = getRandomBlock();

    if (checkCollision(currentBlock)) {
        gameOver = true;
    }
}

bool Engine::checkCollision(const Block& block, int rowOffset) const {
    const Shape& shape = block.getCurrentShape();

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (shape[i][j] == 0) {
                continue;
            }

            int globalRow = block.row + i + rowOffset;
            int globalCol = block.col + j;

            if (globalRow >= Settings::kBoardHeight) {
                return true;
            }
            if (globalCol < 0 || globalCol >= Settings::kBoardWidth) {
                return true;
            }
            if (grid.getCell(globalRow, globalCol) != 0) {
                return true;
            }
        }
    }

    return false;
}

void Engine::lockToGrid() {
    const Shape& shape = currentBlock.getCurrentShape();

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (shape[i][j] != 0) {
                int globalRow = currentBlock.row + i;
                int globalCol = currentBlock.col + j;
                grid.setCell(globalRow, globalCol, static_cast<uint8_t>(currentBlock.type) + 1);
            }
        }
    }

    canHold = true;
}

int Engine::getGhostRow() const {
    int offset = 0;

    while (!checkCollision(currentBlock, offset + 1)) {
        ++offset;
    }

    return currentBlock.row + offset;
}

std::vector<Placement> Engine::getPossiblePlacements() const {
    std::vector<Placement> placements;

    for (int rotationIdx = 0; rotationIdx < kRotationStates; ++rotationIdx) {
        Block testBlock = currentBlock;
        testBlock.rotationIdx = rotationIdx;
        testBlock.row = 0;

        auto [minRow, maxRow, minCol, maxCol] = testBlock.getShapeBounds();
        int minStartCol = -minCol;
        int maxStartCol = Settings::kBoardWidth - 1 - maxCol;

        for (int col = minStartCol; col <= maxStartCol; ++col) {
            testBlock.row = 0;
            testBlock.col = col;

            if (checkCollision(testBlock)) {
                continue;
            }

            while (!checkCollision(testBlock, 1)) {
                ++testBlock.row;
            }

            placements.push_back({rotationIdx, col, testBlock.row});
        }
    }

    return placements;
}

void Engine::updateScore(int linesCleared) {
    switch (linesCleared) {
        case 1: score += 100; break;
        case 2: score += 300; break;
        case 3: score += 500; break;
        case 4: score += 800; break;
        default: break;
    }
}

void Engine::finishDrop() {
    lockToGrid(); // also sets canHold = true

    int linesCleared = grid.clearFullLines();
    if (linesCleared > 0) {
        updateScore(linesCleared);
        totalLinesCleared += linesCleared;
        level = 1 + totalLinesCleared / 10;
    }

    spawnBlock();
}

void Engine::moveDown() {
    if (gameOver) {
        return;
    }

    ++currentBlock.row;

    if (checkCollision(currentBlock)) {
        --currentBlock.row;
        finishDrop();
    }
}

void Engine::moveLeft() {
    if (gameOver) {
        return;
    }

    --currentBlock.col;
    if (checkCollision(currentBlock)) {
        ++currentBlock.col;
    }
}

void Engine::moveRight() {
    if (gameOver) {
        return;
    }

    ++currentBlock.col;
    if (checkCollision(currentBlock)) {
        --currentBlock.col;
    }
}

void Engine::rotateBlock() {
    if (gameOver) {
        return;
    }

    currentBlock.rotate();
    if (checkCollision(currentBlock)) {
        currentBlock.undoRotate();
    }
}

void Engine::hardDrop() {
    if (gameOver) {
        return;
    }

    while (!checkCollision(currentBlock, 1)) {
        ++currentBlock.row;
    }

    finishDrop();
}

void Engine::hold() {
    if (gameOver || !canHold) {
        return;
    }

    if (!holdBlock.has_value()) {
        holdBlock = currentBlock;
        spawnBlock();
    } else {
        std::swap(currentBlock, *holdBlock);
        currentBlock.resetPosition();
    }

    holdBlock->resetPosition();
    canHold = false;
}

void Engine::step(Action action) {
    switch (action) {
        case Action::MoveLeft:  moveLeft(); break;
        case Action::MoveRight: moveRight(); break;
        case Action::Rotate:    rotateBlock(); break;
        case Action::SoftDrop:  moveDown(); break;
        case Action::HardDrop:  hardDrop(); break;
        case Action::Hold:      hold(); break;
    }
}

void Engine::tick() {
    moveDown();
}

const Grid& Engine::getGrid() const { return grid; }
const Block& Engine::getCurrentBlock() const { return currentBlock; }
const Block& Engine::getNextBlock() const { return nextBlock; }
const std::optional<Block>& Engine::getHoldBlock() const { return holdBlock; }
int Engine::getScore() const { return score; }
int Engine::getLevel() const { return level; }
bool Engine::isGameOver() const { return gameOver; }
