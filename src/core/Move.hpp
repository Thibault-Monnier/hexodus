#pragma once
#include "Chunk.hpp"

class Move {
    Coordinate coord1_;
    Coordinate coord2_;

   public:
    Move(const Coordinate coord1, const Coordinate coord2) : coord1_(coord1), coord2_(coord2) {}
    Move() = default;

    [[nodiscard]] Coordinate getCoord1() const { return coord1_; }
    [[nodiscard]] Coordinate getCoord2() const { return coord2_; }
};
