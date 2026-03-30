#include "Engine.hpp"

#include <ranges>

Move Engine::findBestMove(const uint16_t depth) {
    // The transposition table is useless between calls: the number of pieces on the board is
    // strictly increasing, so if the board has advanced, all stored boards while have lower
    // search depths.
    counter_ = 0;
    transpositionTable_.clear();

    initialDepth_ = depth;

    const auto time = std::chrono::high_resolution_clock::now();

    Move bestMove;
    const Score eval = minimax(depth, std::numeric_limits<Score>::min(),
                               std::numeric_limits<Score>::max(), &bestMove);

    const auto endTime = std::chrono::high_resolution_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - time).count();
    std::cout << "Evaluation: " << eval << "\n";
    std::cout << "Used transposition table " << counter_ << " times.\n";
    std::cout << "Time taken: " << duration << " ms\n";
    return bestMove;
}

Score Engine::minimax(const uint16_t remainingDepth, Score alpha, Score beta, Move* bestMoveOut) {
    if (remainingDepth == 0 || board_.endOfGame() != EndOfGameType::None) {
        return evaluate(remainingDepth);
    }

    // NOTE: bestMoveOut is only used for the root node, so we don't have to worry about populating
    // it when using transposition table since TT can't fire on the root node (it is reset between
    // calls to findBestMove). This is fragile but works :-).

    const auto it = transpositionTable_.find(board_.hash());
    if (it != transpositionTable_.end() && it->second.remainingDepth >= remainingDepth) {
        counter_++;
        const Score evaluation = it->second.score;

        if (it->second.flag == TTFlag::LowerBound)
            alpha = std::max(alpha, evaluation);
        else if (it->second.flag == TTFlag::UpperBound)
            beta = std::min(beta, evaluation);

        if (it->second.flag == TTFlag::Exact || alpha >= beta) return evaluation;
    }

    const Score originalAlpha = alpha, originalBeta = beta;

    const bool white = board_.isWhiteToMove();
    Score bestEvaluation =
        white ? std::numeric_limits<Score>::min() : std::numeric_limits<Score>::max();

    const std::vector<Move> moves = board_.possibleMoves();

    for (const Move move : moves) {
        board_.makeMove(move);
        const Score evaluation = minimax(remainingDepth - 1, alpha, beta, nullptr);
        board_.undoMove();

        if (white)
            alpha = std::max(alpha, evaluation);
        else
            beta = std::min(beta, evaluation);

        if (white ? evaluation > bestEvaluation : evaluation < bestEvaluation) {
            bestEvaluation = evaluation;
            if (bestMoveOut) *bestMoveOut = move;
        }

        if (alpha >= beta) break;
    }

    const TTFlag flag = (bestEvaluation <= originalAlpha)  ? TTFlag::UpperBound
                        : (bestEvaluation >= originalBeta) ? TTFlag::LowerBound
                                                           : TTFlag::Exact;
    transpositionTable_[board_.hash()] =
        TTEntry{.remainingDepth = remainingDepth, .score = bestEvaluation, .flag = flag};

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

    const auto& alignments = board_.alignments();

    Score whiteScore = 0, blackScore = 0;
    for (const auto& startEnd : alignments | std::views::keys) {
        const auto& [start, end] = startEnd;
        const uint16_t length = std::max(std::abs(end.x - start.x), std::abs(end.y - start.y)) + 1;
        const TileKind kind = board_.get(start.x, start.y);
        const Score value = static_cast<Score>(length * length);
        if (kind == TileKind::White)
            whiteScore += value;
        else
            blackScore += value;
    }

    return static_cast<Score>(whiteScore - blackScore);
}
