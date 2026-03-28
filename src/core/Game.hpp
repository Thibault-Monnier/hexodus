#pragma once

#include <vector>

#include "Board.hpp"
#include "Move.hpp"
#include "engine/Engine.hpp"

/// Represents the state of the game.
class Game {
    Board board_;

    Engine engine_;

    std::vector<Move> moveHistory_;

   public:
    Game() : engine_(board_) {
        board_.makeMove({.x = 0, .y = 0});  // Place the first piece at the center of the board.
    }

    Move findBestMove(const uint16_t depth) { return engine_.findBestMove(depth); }

    Move playBestMove(const uint16_t depth) {
        const Move bestMove = findBestMove(depth);
        makeMove(bestMove.coord1, bestMove.coord2);
        return bestMove;
    }

    void makeMove(const Coordinate coord1, const Coordinate coord2) {
        const Move move(coord1, coord2);
        board_.makeMove(move);
        moveHistory_.push_back(move);
    }

    [[nodiscard]] const Board& getBoard() const { return board_; }
};
