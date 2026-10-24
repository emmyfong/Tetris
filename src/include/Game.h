#pragma once

#include "Engine.h"
#include "Renderer.h"

struct GLFWwindow;

class GlfwWindowOwner {
public:
    explicit GlfwWindowOwner(GLFWwindow* window);
    ~GlfwWindowOwner();

    GlfwWindowOwner(const GlfwWindowOwner&) = delete;
    GlfwWindowOwner& operator=(const GlfwWindowOwner&) = delete;

    GLFWwindow* get() const;

private:
    GLFWwindow* window;
};

class Game {
public:
    Game();

    void run();

private:
    void processInput();

    GlfwWindowOwner window;
    Engine engine;
    Renderer renderer;
    bool running;
    double lastGravityTime;

    bool leftKeyWasDown;
    bool rightKeyWasDown;
    bool rotateKeyWasDown;
    bool hardDropKeyWasDown;
    bool holdKeyWasDown;
};
