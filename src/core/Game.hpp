#pragma once

#include <vector>

#include "board/Board.hpp"
#include "board/Move.hpp"
#include "engine/Engine.hpp"

/// Represents the state of the game.
class Game {
    Board board_;

    Engine engine_;

   public:
    Game() : engine_(board_) { reset(); }

    Move findBestMove(const uint16_t depth) { return engine_.findBestMove(depth); }

    Move playBestMove(const uint16_t depth) {
        const Move bestMove = findBestMove(depth);
        makeMove(bestMove.coord1, bestMove.coord2);
        return bestMove;
    }

    void makeMove(const Coordinate coord1, const Coordinate coord2) {
        const Move move(coord1, coord2);
        board_.makeMove(move);
    }

    void undoMove() { board_.undoMove(); }

    void reset() {
        board_ = Board();
        board_.makeMove({0, 0});
        engine_.reset();
    }

    [[nodiscard]] const Board& getBoard() const { return board_; }
};
