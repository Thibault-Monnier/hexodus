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

    std::optional<Move> ttMove;
    bool ttCollision = false;

    const TTEntry& entry = transpositionTable_[board_.hash() % transpositionTable_.size()];
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
                if (bestMoveOut) *bestMoveOut = entry.bestMove;
                return eval;
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

    if (ttMove) {
        if (searchMove(ttMove.value(), remainingDepth, alpha, beta, bestEvaluation,
                       bestMoveInThisNode, bestMoveOut)) {
            goto endLoop;
        }
    }

    {
        const Board::CandidateSet& candidates = board_.getCandidates();
        possibleMovesCounter_++;

        const auto getMoves = [&](const auto& predicate) {
            std::vector<Move> moves;
            for (size_t i = 0; i < candidates.size(); ++i) {
                const Coordinate tile1 = candidates[i];
                for (size_t j = i + 1; j < candidates.size(); ++j) {
                    const Coordinate tile2 = candidates[j];
                    assert(tile1 != tile2);

                    if (predicate(tile1, tile2)) {
                        moves.emplace_back(tile1, tile2);
                    }
                }
            }
            return moves;
        };

        const auto searchStage = [&](const auto& predicate) {
            const std::vector<Move> moves = getMoves(predicate);
            generatedMovesCounter_ += moves.size();
            return searchMoves(moves, remainingDepth, alpha, beta, bestEvaluation,
                               bestMoveInThisNode, bestMoveOut);
        };

        searchStage([](const Coordinate& a, const Coordinate& b) {
            return HexCoordinates::areAdjacent(a, b);
        }) ||
            searchStage([](const Coordinate& a, const Coordinate& b) {
                return !HexCoordinates::areAdjacent(a, b);
            });
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

bool Engine::searchMoves(const std::vector<Move>& moves, const uint16_t remainingDepth,
                         Score& alpha, Score& beta, Score& bestEvaluation, Move& bestMoveInThisNode,
                         Move* bestMoveOut) {
    for (const Move move : moves) {
        if (searchMove(move, remainingDepth, alpha, beta, bestEvaluation, bestMoveInThisNode,
                       bestMoveOut)) {
            return true;
        }
    }
    return false;
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
        alphaBetaCutoffs_++;
        return true;
    }

    return false;
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
