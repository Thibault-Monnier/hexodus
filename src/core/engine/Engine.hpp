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
    static constexpr uint64_t TT_SIZE = 5'000'000;

    Board& board_;

    std::vector<TTEntry> transpositionTable_ = std::vector(TT_SIZE, TTEntry{});

    std::array<std::array<Move, 5>, 20> killerMoves_{};

    uint16_t initialDepth_ = 0;

    uint32_t nodesEvaluated_ = 0;
    uint32_t alphaBetaCutoffs_ = 0;
    uint32_t possibleMovesCounter_ = 0;
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
        madeMovesCounter_ = 0;
        ttHitCounter_ = 0;
        ttCollisionCounter_ = 0;
    }

    Score minimax(uint16_t remainingDepth, Score alpha, Score beta, Move* bestMoveOut);

    bool searchMove(Move move, uint16_t remainingDepth, Score& alpha, Score& beta,
                    Score& bestEvaluation, Move& bestMoveInThisNode, Move* bestMoveOut);

    /// Probes the transposition table for the current board state. Returns a Score if a valid entry
    /// is found, std::nullopt otherwise.
    std::optional<Score> probeTT(uint16_t remainingDepth, Score& alpha, Score& beta,
                                 std::optional<Move>& ttMove, bool& ttCollision);

    /// Stores an entry in the transposition table for the current board state.
    void storeTT(uint16_t remainingDepth, Score originalAlpha, Score originalBeta,
                 Score bestEvaluation, Move bestMove, bool ttCollision);

    /// Evaluates the current board state and returns an evaluation score. Positive scores indicate
    /// an advantage for the current player, while negative scores indicate a disadvantage.
    [[nodiscard]] Score evaluate(uint16_t remainingDepth) const;

    [[nodiscard]] bool isWinScore(const Score score) const {
        return std::abs(score) >= WIN_SCORE - initialDepth_;
    }

    [[nodiscard]] TTEntry& getTTEntry() {
        return transpositionTable_[board_.hash() % transpositionTable_.size()];
    }
};
