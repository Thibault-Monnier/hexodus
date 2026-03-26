#pragma once

#include <unordered_map>

#include "Chunk.hpp"

class Board {
    std::unordered_map<Coordinate, Chunk> board_;

   public:
    [[nodiscard]] const std::unordered_map<Coordinate, Chunk>& get() { return board_; }

    [[nodiscard]] bool isOccupied(int x, int y) const;

    void set(TileKind kind, int x, int y);

    void print()const;
};
