#pragma once

#include <cstdint>

// A tiny hand-authored 5x7 pixel ("dot matrix") font covering space,
// digits, and uppercase letters - just enough for SCORE/NEXT/HOLD style
// UI labels and the score number. Each glyph row is 5 bits, bit 4 =
// leftmost pixel, bit 0 = rightmost pixel.
namespace Font {

constexpr int kGlyphWidth = 5;
constexpr int kGlyphHeight = 7;

struct Glyph {
    uint8_t rows[kGlyphHeight];
};

// Every supported character, in atlas order. Index into this string is
// also the glyph's column index in the generated font atlas texture.
constexpr char kGlyphOrder[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr int kGlyphCount = sizeof(kGlyphOrder) - 1; // exclude the null terminator

inline int getGlyphIndex(char c) {
    for (int i = 0; i < kGlyphCount; ++i) {
        if (kGlyphOrder[i] == c) {
            return i;
        }
    }
    return 0; // space, if the character isn't supported
}

inline const Glyph& getGlyph(char c) {
    static constexpr Glyph kSpace = {{0, 0, 0, 0, 0, 0, 0}};
    static constexpr Glyph k0 = {{0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}};
    static constexpr Glyph k1 = {{0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}};
    static constexpr Glyph k2 = {{0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}};
    static constexpr Glyph k3 = {{0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110}};
    static constexpr Glyph k4 = {{0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}};
    static constexpr Glyph k5 = {{0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}};
    static constexpr Glyph k6 = {{0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}};
    static constexpr Glyph k7 = {{0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}};
    static constexpr Glyph k8 = {{0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}};
    static constexpr Glyph k9 = {{0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100}};
    static constexpr Glyph kA = {{0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}};
    static constexpr Glyph kB = {{0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}};
    static constexpr Glyph kC = {{0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110}};
    static constexpr Glyph kD = {{0b11100, 0b10010, 0b10001, 0b10001, 0b10001, 0b10010, 0b11100}};
    static constexpr Glyph kE = {{0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}};
    static constexpr Glyph kF = {{0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}};
    static constexpr Glyph kG = {{0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111}};
    static constexpr Glyph kH = {{0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}};
    static constexpr Glyph kI = {{0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}};
    static constexpr Glyph kJ = {{0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b10001, 0b01110}};
    static constexpr Glyph kK = {{0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001}};
    static constexpr Glyph kL = {{0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}};
    static constexpr Glyph kM = {{0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001}};
    static constexpr Glyph kN = {{0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001}};
    static constexpr Glyph kO = {{0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}};
    static constexpr Glyph kP = {{0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}};
    static constexpr Glyph kQ = {{0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101}};
    static constexpr Glyph kR = {{0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001}};
    static constexpr Glyph kS = {{0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110}};
    static constexpr Glyph kT = {{0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}};
    static constexpr Glyph kU = {{0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}};
    static constexpr Glyph kV = {{0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}};
    static constexpr Glyph kW = {{0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010}};
    static constexpr Glyph kX = {{0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001}};
    static constexpr Glyph kY = {{0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100}};
    static constexpr Glyph kZ = {{0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111}};

    switch (c) {
        case '0': return k0;
        case '1': return k1;
        case '2': return k2;
        case '3': return k3;
        case '4': return k4;
        case '5': return k5;
        case '6': return k6;
        case '7': return k7;
        case '8': return k8;
        case '9': return k9;
        case 'A': return kA;
        case 'B': return kB;
        case 'C': return kC;
        case 'D': return kD;
        case 'E': return kE;
        case 'F': return kF;
        case 'G': return kG;
        case 'H': return kH;
        case 'I': return kI;
        case 'J': return kJ;
        case 'K': return kK;
        case 'L': return kL;
        case 'M': return kM;
        case 'N': return kN;
        case 'O': return kO;
        case 'P': return kP;
        case 'Q': return kQ;
        case 'R': return kR;
        case 'S': return kS;
        case 'T': return kT;
        case 'U': return kU;
        case 'V': return kV;
        case 'W': return kW;
        case 'X': return kX;
        case 'Y': return kY;
        case 'Z': return kZ;
        default: return kSpace;
    }
}

} // namespace Font
