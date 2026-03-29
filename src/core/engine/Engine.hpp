#pragma once

#include <unordered_map>
#include <vector>

#include "core/board/Board.hpp"

using Score = int16_t;

struct TTEntry {
    uint16_t depth;
    Score score;
};

class Engine {
    Board& board_;
    std::vector<Move> moveHistory_;

    std::unordered_map<std::string, TTEntry> transpositionTable_;

   public:
    explicit Engine(Board& board) : board_(board) {}

    void reset() {
        moveHistory_.clear();
        transpositionTable_.clear();
    }

    /// Uses a minimax search algorithm to determine the best move for the current player.
    Move findBestMove(const uint16_t depth) {
        Move bestMove;
        minimax(depth, std::numeric_limits<Score>::min(), std::numeric_limits<Score>::max(),
                &bestMove);
        return bestMove;
    }

   private:
    Score minimax(uint16_t remainingDepth, Score alpha, Score beta, Move* bestMoveOut);

    /// Evaluates the current board state and returns an evaluation score. Positive scores indicate
    /// an advantage for the current player, while negative scores indicate a disadvantage.
    [[nodiscard]] Score evaluate() const;
};
