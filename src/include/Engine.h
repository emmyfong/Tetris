#pragma once

#include <optional>
#include <random>
#include <vector>

#include "Block.h"
#include "Blocks.h"
#include "Grid.h"

enum class Action { MoveLeft, MoveRight, Rotate, SoftDrop, HardDrop, Hold };

class Engine {
public:
    explicit Engine(unsigned int seed);

    void step(Action action);
    void tick(); //gravity driven drop

    const Grid& getGrid() const;
    const Block& getCurrentBlock() const;
    int getScore() const;
    bool isGameOver() const;

private:
    Block getRandomBlock();
    void spawnBlock();
    bool checkCollision(int rowOffset = 0) const;
    int getGhostRow() const;
    void updateScore(int linesClear);
    void moveDown();
    void lockToGrid();
    void finishDrop();

    void moveLeft();
    void moveRight();
    void rotateBlock();
    void hardDrop();
    void hold();

    Grid grid;
    std::vector<BlockType> bag;
    std::mt19937 rng;

    Block currentBlock;
    Block nextBlock;
    std::optional<Block> holdBlock;
    bool canHold;

    int score;
    bool gameOver;
};