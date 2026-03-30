#pragma once

#include <iostream>
#include <vector>

#include "core/board/Board.hpp"

using Score = int16_t;

enum class TTFlag : uint8_t { Exact, LowerBound, UpperBound };

struct TTEntry {
    Move bestMove;
    uint16_t remainingDepth;
    Score score;
    TTFlag flag;
};

class Engine {
    static constexpr Score WIN_SCORE = 10'000;

    Board& board_;

    ankerl::unordered_dense::map<size_t, TTEntry> transpositionTable_;

    uint16_t initialDepth_ = 0;

    int counter_ = 0;

   public:
    explicit Engine(Board& board) : board_(board) {}

    void reset() { transpositionTable_.clear(); }

    /// Uses a minimax search algorithm to determine the best move for the current player.
    Move findBestMove(uint16_t depth);

   private:
    Score minimax(uint16_t remainingDepth, Score alpha, Score beta, Move* bestMoveOut);

    /// Evaluates the current board state and returns an evaluation score. Positive scores indicate
    /// an advantage for the current player, while negative scores indicate a disadvantage.
    [[nodiscard]] Score evaluate(uint16_t remainingDepth) const;
};
