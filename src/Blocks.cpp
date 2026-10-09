#include "Blocks.h"

static constexpr std::array<Shape, 4> kShapesT = {
    Shape{{ {0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0} }}, // rotation 0
    Shape{{ {0,1,0,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0} }}, // rotation 1
    Shape{{ {0,0,0,0}, {1,1,1,0}, {0,1,0,0}, {0,0,0,0} }}, // rotation 2
    Shape{{ {0,1,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0} }}  // rotation 3
};

static constexpr std::array<Shape, 4> kShapesI = {
    Shape{{ {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} }},
    Shape{{ {0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0} }},
    Shape{{ {0,0,0,0}, {0,0,0,0}, {1,1,1,1}, {0,0,0,0} }},
    Shape{{ {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} }}
};

static constexpr std::array<Shape, 4> kShapesO = {
    Shape{{ {1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} }},
    Shape{{ {1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} }},
    Shape{{ {1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} }},
    Shape{{ {1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} }}
};

static constexpr std::array<Shape, 4> kShapesS = {
    Shape{{ {0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} }},
    Shape{{ {0,1,0,0}, {0,1,1,0}, {0,0,1,0}, {0,0,0,0} }},
    Shape{{ {0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0} }},
    Shape{{ {1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0} }}
};

static constexpr std::array<Shape, 4> kShapesZ = {
    Shape{{ {1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0} }},
    Shape{{ {0,0,1,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0} }},
    Shape{{ {0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0} }},
    Shape{{ {0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0} }}
};

static constexpr std::array<Shape, 4> kShapesJ = {
    Shape{{ {1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0} }},
    Shape{{ {0,1,1,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0} }},
    Shape{{ {0,0,0,0}, {1,1,1,0}, {0,0,1,0}, {0,0,0,0} }},
    Shape{{ {0,1,0,0}, {0,1,0,0}, {1,1,0,0}, {0,0,0,0} }}
};

static constexpr std::array<Shape, 4> kShapesL = {
    Shape{{ {0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0} }},
    Shape{{ {0,1,0,0}, {0,1,0,0}, {0,1,1,0}, {0,0,0,0} }},
    Shape{{ {0,0,0,0}, {1,1,1,0}, {1,0,0,0}, {0,0,0,0} }},
    Shape{{ {1,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0} }}
};

const Shape& Blocks::getShape(BlockType type, int rotationIndex) {
    switch (type) {
        case BlockType::I: return kShapesI[rotationIndex];
        case BlockType::O: return kShapesO[rotationIndex];
        case BlockType::T: return kShapesT[rotationIndex];
        case BlockType::S: return kShapesS[rotationIndex];
        case BlockType::Z: return kShapesZ[rotationIndex];
        case BlockType::J: return kShapesJ[rotationIndex];
        case BlockType::L: return kShapesL[rotationIndex];
    }
    return kShapesI[0]; 
}

Settings::Color Blocks::getColor(BlockType type) {
    switch (type) {
        case BlockType::I: return Settings::Color::Cyan;
        case BlockType::O: return Settings::Color::Yellow;
        case BlockType::T: return Settings::Color::Purple;
        case BlockType::S: return Settings::Color::Green;
        case BlockType::Z: return Settings::Color::Red;
        case BlockType::J: return Settings::Color::Blue;
        case BlockType::L: return Settings::Color::Orange;
    }
    return Settings::Color::Red;
}
