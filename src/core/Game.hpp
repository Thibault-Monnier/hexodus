#pragma once

#include "Board.hpp"

class Game {
    Board board_;

    bool whiteToMove_ = true;

   public:
    [[nodiscard]] const Board& getBoard() const { return board_; }
};
