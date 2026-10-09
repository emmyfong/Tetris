#include "Engine.h"

#include <algorithm>
#include <utility>

Engine::Engine(unsigned int seed)
    : rng(seed),
      currentBlock(BlockType::I),
      nextBlock(BlockType::I),
      canHold(true),
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

    if (checkCollision()) {
        gameOver = true;
    }
}

bool Engine::checkCollision(int rowOffset) const {
    const Shape& shape = currentBlock.getCurrentShape();

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (shape[i][j] == 0) {
                continue;
            }

            int globalRow = currentBlock.row + i + rowOffset;
            int globalCol = currentBlock.col + j;

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

    while (!checkCollision(offset + 1)) {
        ++offset;
    }

    return currentBlock.row + offset;
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
    }

    spawnBlock();
}

void Engine::moveDown() {
    if (gameOver) {
        return;
    }

    ++currentBlock.row;

    if (checkCollision()) {
        --currentBlock.row;
        finishDrop();
    }
}

void Engine::moveLeft() {
    if (gameOver) {
        return;
    }

    --currentBlock.col;
    if (checkCollision()) {
        ++currentBlock.col;
    }
}

void Engine::moveRight() {
    if (gameOver) {
        return;
    }

    ++currentBlock.col;
    if (checkCollision()) {
        --currentBlock.col;
    }
}

void Engine::rotateBlock() {
    if (gameOver) {
        return;
    }

    currentBlock.rotate();
    if (checkCollision()) {
        currentBlock.undoRotate();
    }
}

void Engine::hardDrop() {
    if (gameOver) {
        return;
    }

    while (!checkCollision(1)) {
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
int Engine::getScore() const { return score; }
bool Engine::isGameOver() const { return gameOver; }
