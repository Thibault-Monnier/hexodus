#pragma once

#include "core/board/Board.hpp"

using Score = int16_t;

enum class TTFlag : uint8_t { Exact, LowerBound, UpperBound };

struct TTEntry {
    Move bestMove;
    uint16_t remainingDepth;
    Score score;
    TTFlag flag;
    uint64_t hash;
};

class Engine {
    static constexpr Score WIN_SCORE = 10'000;
    static constexpr uint64_t TT_SIZE = 10'000'000;

    Board& board_;

    std::vector<TTEntry> transpositionTable_ = std::vector(TT_SIZE, TTEntry{});
    uint16_t initialDepth_ = 0;

    uint32_t nodesEvaluated_ = 0;
    uint32_t alphaBetaCutoffs_ = 0;
    uint32_t possibleMovesCounter_ = 0;
    uint32_t generatedMovesCounter_ = 0;
    uint32_t madeMovesCounter_ = 0;
    uint32_t ttHitCounter_ = 0;
    uint32_t ttCollisionCounter_ = 0;

   public:
    explicit Engine(Board& board) : board_(board) {}

    void reset() {
        transpositionTable_ = std::vector(TT_SIZE, TTEntry{});
        initialDepth_ = 0;
        resetCounters();
    }

    /// Uses a minimax search algorithm to determine the best move for the current player.
    Move findBestMove(uint16_t depth);

   private:
    void resetCounters() {
        nodesEvaluated_ = 0;
        alphaBetaCutoffs_ = 0;
        possibleMovesCounter_ = 0;
        generatedMovesCounter_ = 0;
        ttHitCounter_ = 0;
        ttCollisionCounter_ = 0;
    }

    Score minimax(uint16_t remainingDepth, Score alpha, Score beta, Move* bestMoveOut);

    /// Evaluates the current board state and returns an evaluation score. Positive scores indicate
    /// an advantage for the current player, while negative scores indicate a disadvantage.
    [[nodiscard]] Score evaluate(uint16_t remainingDepth) const;
};
