#pragma once

#include "Coordinate.hpp"

struct Move {
    Coordinate coord1;
    Coordinate coord2;

    Move() = default;
    Move(const Coordinate coord1, const Coordinate coord2) : coord1(coord1), coord2(coord2) {}

    bool operator==(const Move& move) const = default;
};
