#include <GLFW/glfw3.h>

int main() {
    if (!glfwInit()) {
        return 1;
    }

    // GLFW defaults to setting up an OpenGL context on window creation —
    // we're using Vulkan instead, so tell it not to bother.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Tetris - Window Test", nullptr, nullptr);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
