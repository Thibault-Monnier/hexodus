#pragma once

#include <unordered_map>

#include "Chunk.hpp"

class Board {
    std::unordered_map<Coordinate, Chunk> board_;

   public:
    [[nodiscard]] const std::unordered_map<Coordinate, Chunk>& get() { return board_; }

    [[nodiscard]] bool isOccupied(int16_t x, int16_t y) const;

    void set(TileKind kind, int16_t x, int16_t y);

    void print()const;
};
