#include "Engine.hpp"

Score Engine::minimax(const uint16_t remainingDepth, Score alpha, Score beta, Move* bestMoveOut) {
    if (remainingDepth == 0 || board_.endOfGame() != EndOfGameType::None) {
        return evaluate();
    }

    const std::vector<Move> moves = board_.possibleMoves();

    const bool white = board_.isWhiteToMove();

    Score bestEvaluation =
        white ? std::numeric_limits<Score>::min() : std::numeric_limits<Score>::max();
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
        return (board_.isWhiteToMove()) ? std::numeric_limits<Score>::min() + 1
                                        : std::numeric_limits<Score>::max() - 1;
    }

    // TODO: Improve this using the alignments_ vector.
    return 0;
}
