#pragma once

#include <ankerl/unordered_dense.h>

#include <cassert>
#include <vector>

#include "Coordinate.hpp"
#include "Move.hpp"

enum class TileKind : uint8_t { Empty, Black, White };

enum class EndOfGameType : uint8_t { None, Win, Draw };

/// Represents the state of a board, handles move validation and generation. Keeps track of various
/// things to allow for efficient use in the engine.
class Board {
   public:
    static constexpr size_t SIZE = 128;

   private:
    bool whiteToMove_ = true;

    std::array<std::array<TileKind, SIZE>, SIZE> board_ = {};

    /// Stores the start and end coordinates of every line of 2 or more consecutive aligned
    /// pieces of the same color, along with the coordinate of the piece that added the alignment.
    /// Used to efficiently check for end-of-game conditions and for position evaluation in the
    /// engine.
    std::vector<std::pair<std::pair<Coordinate, Coordinate>, Coordinate>> alignments_;

    /// Stores the history of moves made, used for undoing moves and possible moves generation.
    std::vector<Move> moveHistory_;

    /// Stores a Zobrist hash of the current board state, used for the transposition table in the
    /// engine. Updated incrementally after every move.
    uint64_t zobristHash_ = 0;

   public:
    int counter = 0;

    [[nodiscard]] bool isOccupied(int16_t x, int16_t y) const;
    [[nodiscard]] bool isWhiteToMove() const { return whiteToMove_; }
    [[nodiscard]] uint64_t hash() const { return zobristHash_; }

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

   private:
    void set(TileKind kind, int16_t x, int16_t y);

    [[nodiscard]] Coordinate findAlignmentEnd(Coordinate origin, int16_t dx, int16_t dy,
                                              TileKind kind) const;

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

    /// Updates the Zobrist hash by XORing the hash value for the given tile. This can be used
    /// either after doing or undoing a move.
    void updateHash(int16_t x, int16_t y, TileKind kind);
};
