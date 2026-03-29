#pragma once

#include <ranges>
#include <unordered_map>
#include <vector>

#include "Chunk.hpp"
#include "Move.hpp"

enum class EndOfGameType : uint8_t { None, Win, Draw };

/// Represents the state of a board, handles move validation and generation. Keeps track of various
/// things to allow for efficient use in the engine.
class Board {
    bool whiteToMove_ = true;
    std::unordered_map<Coordinate, Chunk> board_;

    /// Stores the start and end coordinates of every line of 2 or more consecutive aligned
    /// pieces of the same color, along with the coordinate of the piece that added the alignment.
    /// Used to efficiently check for end-of-game conditions and for position evaluation in the
    /// engine.
    std::vector<std::pair<std::pair<Coordinate, Coordinate>, Coordinate>> alignments_;

    /// Stores the history of moves made, used for undoing moves and possible moves generation.
    std::vector<Move> moveHistory_;

    /// Stores a hash of the current board state, used for the transposition table in the engine.
    /// Updated after every move.
    std::string hash_;

   public:
    int counter = 0;

    [[nodiscard]] bool isOccupied(int16_t x, int16_t y) const;
    [[nodiscard]] bool isWhiteToMove() const { return whiteToMove_; }

    [[nodiscard]] EndOfGameType endOfGame() const;

    /// Updates the board state by placing pieces according to the move. If the move is invalid,
    /// prints an error message and does not update the board.
    void makeMove(Move move);
    /// Single piece move for the first move of the game.
    void makeMove(Coordinate coord1);

    /// Reverts the board state to before the last move was made.
    void undoMove();

    /// Returns a list of all valid moves for the current player.
    [[nodiscard]] std::vector<Move> possibleMoves() const;

    /// Prints a representation of the board to the console.
    void print() const;

    /// Returns a flattened view of all the tiles on the board
    [[nodiscard]] auto getTiles() const {
        return board_ | std::views::values |
               std::views::transform([](const Chunk& chunk) { return chunk.getFlat(); }) |
               std::views::join;
    }

   private:
    void set(TileKind kind, int16_t x, int16_t y);

    Coordinate findAlignmentEnd(Coordinate origin, int16_t dx, int16_t dy, TileKind kind);

    /// Checks if the move is valid. If not, prints an error message.
    [[nodiscard]] bool validateMove(const Move& move) const;

    [[nodiscard]] Move lastMove() const {
        assert(!moveHistory_.empty());
        return moveHistory_.back();
    }

    [[nodiscard]] Move beforeLastMove() const {
        assert(moveHistory_.size() >= 2);
        return moveHistory_[moveHistory_.size() - 2];
    }

    /// Returns the last move made and removes if from the move history.
    [[nodiscard]] Move popLastMove() {
        const Move move = lastMove();
        moveHistory_.pop_back();
        return move;
    }
};

template <>
struct std::hash<Board> {
    size_t operator()(const Board& board) const noexcept {
        size_t v = 0;
        for (auto tile : board.getTiles()) {
            v ^= std::hash<int>{}(static_cast<int>(tile)) + 0x9e3779b9 + (v << 6) + (v >> 2);
        }
        return v;
    }
};
