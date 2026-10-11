#include "Game.h"
#include <iostream>
#include <random>

Game::Game() : engine(std::random_device{}()), running(true) {
}

void Game::run() {
    std::cout << "Controls: a=left d=right s=soft-drop w=rotate x=hard-drop c=hold q=quit\n";
    std::cout << "(Each keypress also advances gravity by one row for now; no real timer yet.)\n\n";

    while (running && !engine.isGameOver()) {
        renderer.draw(engine);
        handleInput();
    }

    renderer.draw(engine);
    std::cout << "Final score: " << engine.getScore() << '\n';
}

void Game::handleInput() {
    char key;
    std::cin >> key;

    switch (key) {
        case 'a': engine.step(Action::MoveLeft); break;
        case 'd': engine.step(Action::MoveRight); break;
        case 's': engine.step(Action::SoftDrop); break;
        case 'w': engine.step(Action::Rotate); break;
        case 'x': engine.step(Action::HardDrop); break;
        case 'c': engine.step(Action::Hold); break;
        case 'q': running = false; break;
        default: break;
    }

    engine.tick();
}
