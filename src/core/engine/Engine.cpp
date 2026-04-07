#include "Engine.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <ranges>

Move Engine::findBestMove(const uint16_t depth) {
    resetCounters();

    const auto startTime = std::chrono::high_resolution_clock::now();

    Move bestMove;
    Score eval = 0;
    for (uint16_t d = 1; d <= depth; ++d) {
        initialDepth_ = d;
        eval = minimax(d, std::numeric_limits<Score>::min(), std::numeric_limits<Score>::max(),
                       &bestMove);
        std::cout << "Depth: " << d << ", Evaluation: " << eval << "\n";
    }

    const auto endTime = std::chrono::high_resolution_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "Evaluation: " << eval << "\n";
    std::cout << "Best move: (" << bestMove.coord1.x << ", " << bestMove.coord1.y << ") and ("
              << bestMove.coord2.x << ", " << bestMove.coord2.y << ")\n";
    std::cout << "Nodes evaluated: " << nodesEvaluated_ << "\n";
    std::cout << "Alpha-beta cutoffs: " << alphaBetaCutoffs_ << "\n";
    std::cout << "Generated moves: " << generatedMovesCounter_ << "\n";
    std::cout << "Possible moves calls: " << possibleMovesCounter_ << "\n";
    std::cout << "Made moves: " << madeMovesCounter_ << "\n";
    std::cout << "Transposition table hits: " << ttHitCounter_ << "\n";
    std::cout << "Transposition table collisions: " << ttCollisionCounter_ << "\n";
    std::cout << "Time taken: " << duration << " ms\n";

    return bestMove;
}

Score Engine::minimax(const uint16_t remainingDepth, Score alpha, Score beta, Move* bestMoveOut) {
    nodesEvaluated_++;

    if (remainingDepth == 0 || board_.endOfGame() != EndOfGameType::None) {
        return evaluate(remainingDepth);
    }

    Move ttMove;
    bool hasTtMove = false;

    bool ttCollision = false;

    const TTEntry& entry = transpositionTable_[board_.hash() % transpositionTable_.size()];
    if (entry.hash == board_.hash()) {
        ttHitCounter_++;

        ttMove = entry.bestMove;
        hasTtMove = true;

        if (entry.remainingDepth >= remainingDepth) {
            const Score evaluation = entry.score;

            if (entry.flag == TTFlag::LowerBound)
                alpha = std::max(alpha, evaluation);
            else if (entry.flag == TTFlag::UpperBound)
                beta = std::min(beta, evaluation);

            if (entry.flag == TTFlag::Exact || alpha >= beta) {
                if (bestMoveOut) *bestMoveOut = entry.bestMove;
                return evaluation;
            }
        }
    } else if (entry.hash != 0) {
        ttCollisionCounter_++;
        ttCollision = true;
    }

    const Score originalAlpha = alpha, originalBeta = beta;

    const bool white = board_.isWhiteToMove();
    Score bestEvaluation =
        white ? std::numeric_limits<Score>::min() : std::numeric_limits<Score>::max();
    Move bestMoveInThisNode;

    std::vector<Move> moves;
    board_.generatePossibleMoves(moves);
    possibleMovesCounter_++;
    generatedMovesCounter_ += moves.size();

    std::array<std::vector<Move>, 5> buckets;
    for (const Move move : moves) {
        Score score = 0;

        if (hasTtMove && move == ttMove)
            score = 4;
        else {
            const int16_t dist = HexCoordinates::hexDistance(move.coord1, move.coord2);
            if (dist == 1) score = 3;
        }

        buckets[score].push_back(move);
    }

    // Iterate over the sorted moves
    for (const std::vector<Move>& bucket : std::ranges::reverse_view(buckets)) {
        for (const Move move : bucket) {
            madeMovesCounter_++;
            board_.makeMove(move, false);
            const Score evaluation = minimax(remainingDepth - 1, alpha, beta, nullptr);
            board_.undoMove();

            if (white)
                alpha = std::max(alpha, evaluation);
            else
                beta = std::min(beta, evaluation);

            if (white ? evaluation > bestEvaluation : evaluation < bestEvaluation) {
                bestEvaluation = evaluation;
                bestMoveInThisNode = move;
                if (bestMoveOut) *bestMoveOut = move;
            }

            if (alpha >= beta) {
                alphaBetaCutoffs_++;
                goto endLoop;
            }
        }
    }

endLoop:

    const TTFlag flag = (bestEvaluation <= originalAlpha)  ? TTFlag::UpperBound
                        : (bestEvaluation >= originalBeta) ? TTFlag::LowerBound
                                                           : TTFlag::Exact;

    if (!ttCollision || entry.remainingDepth <= remainingDepth) {
        transpositionTable_[board_.hash() % transpositionTable_.size()] = TTEntry{
            .bestMove = bestMoveInThisNode,
            .remainingDepth = remainingDepth,
            .score = bestEvaluation,
            .flag = flag,
            .hash = board_.hash(),
        };
    }

    return bestEvaluation;
}

Score Engine::evaluate(const uint16_t remainingDepth) const {
    const uint16_t depth = initialDepth_ - remainingDepth;

    const EndOfGameType endOfGame = board_.endOfGame();
    if (endOfGame != EndOfGameType::None) [[unlikely]] {
        if (endOfGame == EndOfGameType::Draw) return 0;

        assert(endOfGame == EndOfGameType::Win);
        // If it's the end of the game, the previous player is the winner
        const Score abs = static_cast<Score>(WIN_SCORE - depth);
        return board_.isWhiteToMove() ? -abs : abs;
    }

    Score whiteScore = 0, blackScore = 0;
    for (uint32_t length = 2; length <= GameRuleConstants::WINNING_ALIGNMENT_LENGTH; ++length) {
        const auto value = static_cast<Score>(length * length);
        whiteScore += board_.countAlignments(length, true) * value;
        blackScore += board_.countAlignments(length, false) * value;
    }

    return static_cast<Score>(whiteScore - blackScore);
}
