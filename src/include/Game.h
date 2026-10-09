#pragma once

#include "Engine.h"
#include "Renderer.h"

class Game {
public:
    Game();

    void run();

private:
    void handleInput();

    Engine engine;
    Renderer renderer;
    bool running;
};