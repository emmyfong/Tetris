#pragma once

#include <cstdint>

namespace Settings {
    constexpr int kBoardWidth = 10;
    constexpr int kBoardHeight = 20;

    // Window layout: board occupies the left kBoardPixelWidth pixels,
    // everything right of that is the UI panel (score/next/hold).
    // kBoardPixelWidth * 2 must equal kWindowHeight for cells to render
    // square, since the board is kBoardWidth x kBoardHeight = 10x20 (a 1:2
    // ratio) and occupies the window's full height.
    constexpr int kWindowWidth = 750;
    constexpr int kWindowHeight = 800;
    constexpr int kBoardPixelWidth = 400;

    enum class Color { Yellow, Purple, Cyan, Blue, Orange, Green, Red };

    struct RGB {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    constexpr RGB getColorRGB(Color color) {
        switch (color) {
            case Color::Yellow: return {255, 255, 0};
            case Color::Purple: return {128, 0, 128};
            case Color::Cyan:   return {0, 255, 255};
            case Color::Blue:   return {0, 0, 255};
            case Color::Orange: return {255, 165, 0};
            case Color::Green:  return {0, 255, 0};
            case Color::Red:    return {255, 0, 0};
        }
        return {0, 0, 0};
    }
}