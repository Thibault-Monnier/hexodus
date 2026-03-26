#pragma once

#include <unordered_map>

#include "Chunk.hpp"
#include "Move.hpp"

class Board {
    bool whiteToMove_ = true;
    std::unordered_map<Coordinate, Chunk> board_;

   public:
    [[nodiscard]] bool isOccupied(int16_t x, int16_t y) const;

    /// Updates the board state by placing pieces according to the move. If the move is invalid,
    /// prints an error message and does not update the board.
    void makeMove(Move move);
    /// Single piece move for the first move of the game.
    void makeMove(Coordinate coord1);

    /// Prints a representation of the board to the console.
    void print() const;

   private:
    void set(TileKind kind, int16_t x, int16_t y);

    /// Checks if the move is valid. If not, prints an error message.
    [[nodiscard]] bool validateMove(const Move& move) const;
};
