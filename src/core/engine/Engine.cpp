#include "Engine.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <ranges>

#include "core/board/HexCoordinates.hpp"

Move Engine::findBestMove(const uint16_t depth) {
    resetCounters();

    const auto startTime = std::chrono::high_resolution_clock::now();

    Move bestMove;
    Score eval = 0;
    for (uint16_t d = 1; d <= depth; ++d) {
        initialDepth_ = d;
        eval = minimax(d, std::numeric_limits<Score>::min(), std::numeric_limits<Score>::max(),
                       &bestMove);

        const bool won = isWinScore(eval);
        std::cout << "Depth: " << d << ", Evaluation: "
                  << (won ? "Win in " + std::to_string(WIN_SCORE - std::abs(eval)) + " moves"
                          : std::to_string(eval))
                  << "\n";

        if (won) {
            break;
        }
    }

    const auto endTime = std::chrono::high_resolution_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "Evaluation: " << eval << "\n";
    std::cout << "Best move: (" << bestMove.coord1.x << ", " << bestMove.coord1.y << ") and ("
              << bestMove.coord2.x << ", " << bestMove.coord2.y << ")\n";
    std::cout << "Nodes evaluated: " << nodesEvaluated_ << "\n";
    std::cout << "Alpha-beta cutoffs: " << alphaBetaCutoffs_ << "\n";
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

    std::optional<Move> ttMove;
    bool ttCollision = false;

    if (const std::optional<Score> eval =
            probeTT(remainingDepth, alpha, beta, ttMove, ttCollision)) {
        if (bestMoveOut) *bestMoveOut = ttMove.value();
        return eval.value();
    }

    const Score originalAlpha = alpha, originalBeta = beta;
    const bool white = board_.isWhiteToMove();
    Score bestEvaluation =
        white ? std::numeric_limits<Score>::min() : std::numeric_limits<Score>::max();
    Move bestMoveInThisNode;

    // Intentional copy to prevent child nodes from modifying the set while we iterate over it.
    const Board::CandidateSet candidates = board_.getCandidates();
    possibleMovesCounter_++;

    const auto searchStage = [&](const auto& predicate) {
        for (size_t i = 0; i < candidates.size(); ++i) {
            const Coordinate tile1 = candidates[i];
            for (size_t j = i + 1; j < candidates.size(); ++j) {
                const Coordinate tile2 = candidates[j];
                assert(tile1 != tile2);

                if (!predicate(tile1, tile2)) continue;

                const Move move{tile1, tile2};
                if (searchMove(move, remainingDepth, alpha, beta, bestEvaluation,
                               bestMoveInThisNode, bestMoveOut)) {
                    return true;
                }
            }
        }
        return false;
    };

    bool cutoff = (ttMove && searchMove(ttMove.value(), remainingDepth, alpha, beta, bestEvaluation,
                                        bestMoveInThisNode, bestMoveOut));
    if (!cutoff) {
        // Killer moves
        for (const Move& killerMove : killerMoves_[initialDepth_ - remainingDepth]) {
            if (killerMove == Move{}) continue;
            if (!board_.isValidMoveFast(killerMove)) continue;
            if (searchMove(killerMove, remainingDepth, alpha, beta, bestEvaluation,
                           bestMoveInThisNode, bestMoveOut)) {
                cutoff = true;
                break;
            }
        }
    }
    if (!cutoff) cutoff = searchStage(HexCoordinates::areAdjacent);
    if (!cutoff) cutoff = searchStage(std::not_fn(HexCoordinates::areAdjacent));

    storeTT(remainingDepth, originalAlpha, originalBeta, bestEvaluation, bestMoveInThisNode,
            ttCollision);

    return bestEvaluation;
}

bool Engine::searchMove(const Move move, const uint16_t remainingDepth, Score& alpha, Score& beta,
                        Score& bestEvaluation, Move& bestMoveInThisNode, Move* bestMoveOut) {
    const bool white = board_.isWhiteToMove();

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
        const uint16_t depth = initialDepth_ - remainingDepth;
        for (size_t i = killerMoves_[depth].size() - 1; i > 0; --i)
            killerMoves_[depth][i] = killerMoves_[depth][i - 1];
        killerMoves_[depth][0] = move;

        alphaBetaCutoffs_++;
        return true;
    }

    return false;
}

std::optional<Score> Engine::probeTT(const uint16_t remainingDepth, Score& alpha, Score& beta,
                                     std::optional<Move>& ttMove, bool& ttCollision) {
    const TTEntry& entry = getTTEntry();

    if (entry.hash == board_.hash()) {
        ttHitCounter_++;
        ttMove = entry.bestMove;

        if (entry.remainingDepth >= remainingDepth) {
            const auto [flag, eval] = std::make_pair(entry.flag, entry.score);
            if (flag == TTFlag::LowerBound)
                alpha = std::max(alpha, eval);
            else if (flag == TTFlag::UpperBound)
                beta = std::min(beta, eval);

            if (flag == TTFlag::Exact || alpha >= beta) {
                return eval;
            }
        }
    } else if (entry.hash != 0) {
        ttCollisionCounter_++;
        ttCollision = true;
    }

    return std::nullopt;
}

void Engine::storeTT(const uint16_t remainingDepth, const Score originalAlpha,
                     const Score originalBeta, const Score bestEvaluation, const Move bestMove,
                     const bool ttCollision) {
    const TTEntry& entry = getTTEntry();

    const TTFlag flag = (bestEvaluation <= originalAlpha)  ? TTFlag::UpperBound
                        : (bestEvaluation >= originalBeta) ? TTFlag::LowerBound
                                                           : TTFlag::Exact;
    if (!ttCollision || entry.remainingDepth <= remainingDepth) {
        getTTEntry() = TTEntry{
            .bestMove = bestMove,
            .remainingDepth = remainingDepth,
            .score = bestEvaluation,
            .flag = flag,
            .hash = board_.hash(),
        };
    }
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
        whiteScore += board_.countAlignmentsWhite(length) * value;
        blackScore += board_.countAlignmentsBlack(length) * value;
    }

    return static_cast<Score>(whiteScore - blackScore);
}
