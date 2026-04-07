#pragma once

#include "Coordinate.hpp"

struct Move {
    Coordinate coord1;
    Coordinate coord2;

    bool operator==(const Move& move) const = default;
};
