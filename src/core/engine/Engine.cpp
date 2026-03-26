#include "Engine.hpp"

Score Engine::minimax(const uint16_t remainingDepth, Score alpha, Score beta) {
    if (remainingDepth == 0) {
        return evaluate();
    }

    const std::vector<Move> moves = board_.possibleMoves();

    const bool white = board_.isWhiteToMove();

    Score bestEvaluation =
        white ? std::numeric_limits<Score>::min() : std::numeric_limits<Score>::max();
    for (const Move move : moves) {
        board_.makeMove(move);
        const Score evaluation = minimax(remainingDepth - 1, alpha, beta);
        board_.undoMove(move);

        if (white)
            alpha = std::max(alpha, evaluation);
        else
            beta = std::min(beta, evaluation);

        if (evaluation > bestEvaluation) {
            bestEvaluation = evaluation;
            bestLocalMove_ = move;
        }

        if (beta <= alpha) break;
    }

    return bestEvaluation;
}

Score Engine::evaluate() const {
    const EndOfGameType endOfGame = board_.endOfGame();
    if (endOfGame != EndOfGameType::None) [[unlikely]] {
        if (endOfGame == EndOfGameType::Draw) return 0;

        assert(endOfGame == EndOfGameType::Win);
        // If it's the end of the game, the previous player is the winner
        return (board_.isWhiteToMove()) ? std::numeric_limits<Score>::min()
                                        : std::numeric_limits<Score>::max();
    }

    // TODO: Improve this using the alignments_ vector.
    return 0;
}
