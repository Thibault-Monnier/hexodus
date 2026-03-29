#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>

#include "core/board/Board.hpp"

using Score = int16_t;

enum class TTFlag : uint8_t { Exact, LowerBound, UpperBound };

struct TTEntry {
    uint16_t remainingDepth;
    Score score;
    TTFlag flag;
};

class Engine {
    Board& board_;
    std::vector<Move> moveHistory_;

    int counter_ = 0;

    std::unordered_map<size_t, TTEntry> transpositionTable_;

   public:
    explicit Engine(Board& board) : board_(board) {}

    void reset() {
        moveHistory_.clear();
        transpositionTable_.clear();
    }

    /// Uses a minimax search algorithm to determine the best move for the current player.
    Move findBestMove(uint16_t depth);

   private:
    Score minimax(uint16_t remainingDepth, Score alpha, Score beta, Move* bestMoveOut);

    /// Evaluates the current board state and returns an evaluation score. Positive scores indicate
    /// an advantage for the current player, while negative scores indicate a disadvantage.
    [[nodiscard]] Score evaluate() const;
};
