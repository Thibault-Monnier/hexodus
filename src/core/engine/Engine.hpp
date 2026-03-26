#pragma once

#include <unordered_map>
#include <vector>

#include "core/Board.hpp"

using Score = int16_t;

struct TTEntry {
    uint16_t depth;
    Score score;
};

class Engine {
    Board& board_;
    std::vector<Move> moveHistory_;

    const std::unordered_map<Coordinate, TTEntry> transpositionTable_;

    Move bestLocalMove_{};

   public:
    explicit Engine(Board& board) : board_(board) {}

    /// Uses a minimax search algorithm to determine the best move for the current player.
    Move findBestMove(const uint16_t depth) {
        minimax(depth, std::numeric_limits<Score>::min(), std::numeric_limits<Score>::max());
        return bestLocalMove_;
    }

   private:
    Score minimax(uint16_t remainingDepth, Score alpha, Score beta);

    /// Evaluates the current board state and returns an evaluation score. Positive scores indicate
    /// an advantage for the current player, while negative scores indicate a disadvantage.
    [[nodiscard]] Score evaluate() const;
};
