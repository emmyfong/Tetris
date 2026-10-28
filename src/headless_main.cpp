#include "Engine.h"
#include <iostream>
#include <chrono>

int main() {
    Engine engine(std::random_device{}());

    int steps = 0;
    auto start = std::chrono::steady_clock::now();

    while (!engine.isGameOver() && steps < 1'000'000) {
        engine.tick();
        ++steps;
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    double seconds = std::chrono::duration<double>(elapsed).count();
    std::cout << "ran " << steps << " ticks in " << seconds << "s ("
              << steps / seconds << " ticks/sec), final score=" << engine.getScore() << "\n";
}
