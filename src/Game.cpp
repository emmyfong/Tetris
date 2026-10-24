#include "Game.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <random>

namespace {

constexpr double kGravityIntervalSeconds = 0.5;

GLFWwindow* createGlfwWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(500, 1000, "Tetris", nullptr, nullptr); // matches the 10x20 board's 1:2 aspect ratio
}

} // namespace

GlfwWindowOwner::GlfwWindowOwner(GLFWwindow* w) : window(w) {}

GlfwWindowOwner::~GlfwWindowOwner() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

GLFWwindow* GlfwWindowOwner::get() const {
    return window;
}

Game::Game()
    : window(createGlfwWindow()),
      engine(std::random_device{}()),
      renderer(window.get()),
      running(true),
      lastGravityTime(glfwGetTime()),
      leftKeyWasDown(false),
      rightKeyWasDown(false),
      rotateKeyWasDown(false),
      hardDropKeyWasDown(false),
      holdKeyWasDown(false) {
}

void Game::run() {
    while (running && !glfwWindowShouldClose(window.get())) {
        glfwPollEvents();
        processInput();

        double currentTime = glfwGetTime();
        if (currentTime - lastGravityTime >= kGravityIntervalSeconds) {
            engine.tick();
            lastGravityTime = currentTime;
        }

        renderer.draw(engine);
    }
}

void Game::processInput() {
    GLFWwindow* w = window.get();

    bool leftDown = glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS;
    if (leftDown && !leftKeyWasDown) {
        engine.step(Action::MoveLeft);
    }
    leftKeyWasDown = leftDown;

    bool rightDown = glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS;
    if (rightDown && !rightKeyWasDown) {
        engine.step(Action::MoveRight);
    }
    rightKeyWasDown = rightDown;

    bool rotateDown = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS;
    if (rotateDown && !rotateKeyWasDown) {
        engine.step(Action::Rotate);
    }
    rotateKeyWasDown = rotateDown;

    bool hardDropDown = glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (hardDropDown && !hardDropKeyWasDown) {
        engine.step(Action::HardDrop);
    }
    hardDropKeyWasDown = hardDropDown;

    bool holdDown = glfwGetKey(w, GLFW_KEY_C) == GLFW_PRESS;
    if (holdDown && !holdKeyWasDown) {
        engine.step(Action::Hold);
    }
    holdKeyWasDown = holdDown;

    // Soft drop intentionally has no edge-detection: holding the key down
    // should keep dropping, same as real Tetris.
    if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS) {
        engine.step(Action::SoftDrop);
    }

    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        running = false;
    }
}
