#pragma once

#include <vector>

#include "Board.hpp"
#include "Move.hpp"

/// Represents the state of the game and handles the game logic.
class Game {
    Board board_;
    std::vector<Move> moveHistory_;

   public:
    Game() {
        board_.makeMove({.x = 0, .y = 0});  // Place the first piece at the center of the board.
    }

    void makeMove(const Coordinate coord1, const Coordinate coord2) {
        const Move move(coord1, coord2);
        board_.makeMove(move);
        moveHistory_.push_back(move);
    }

    [[nodiscard]] const Board& getBoard() const { return board_; }
};
