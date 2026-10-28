#include "Game.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <random>

namespace {

constexpr double kSoftDropIntervalSeconds = 0.05; // much faster than gravity, but not unthrottled

double gravityIntervalForLevel(int level) {
    double interval = 0.5 - (level - 1) * 0.08;
    return interval < 0.1 ? 0.1 : interval;
}

GLFWwindow* createGlfwWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return glfwCreateWindow(Settings::kWindowWidth, Settings::kWindowHeight, "Tetris", nullptr, nullptr);
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
      lastSoftDropTime(glfwGetTime()),
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
        if (currentTime - lastGravityTime >= gravityIntervalForLevel(engine.getLevel())) {
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

    if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS) {
        double currentTime = glfwGetTime();
        if (currentTime - lastSoftDropTime >= kSoftDropIntervalSeconds) {
            engine.step(Action::SoftDrop);
            lastSoftDropTime = currentTime;
        }
    }

    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        running = false;
    }
}
