#pragma once

#include <vector>

#include "Board.hpp"
#include "Move.hpp"

class Game {
    Board board_;
    std::vector<Move> moveHistory_;

    bool whiteToMove_ = true;

   public:
    Game() {
        makeMove({.x = 0, .y = 0});  // Place the first piece at the center of the board.
    }

    [[nodiscard]] const Board& getBoard() const { return board_; }

    void makeMove(Coordinate coord1, Coordinate coord2);

   private:
    /// Single piece move for the first move of the game.
    void makeMove(Coordinate coord1);

    /// Checks if the move is valid. If not, prints an error message.
    [[nodiscard]] bool validateMove(const Move& move) const;
};
