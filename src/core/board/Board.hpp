#pragma once

#include <ankerl/unordered_dense.h>

#include <cassert>
#include <vector>

#include "Coordinate.hpp"
#include "HexCoordinates.hpp"
#include "Move.hpp"
#include "core/GameRuleConstants.hpp"

enum class TileKind : uint8_t { Empty, Black, White };

enum class EndOfGameType : uint8_t { None, Win, Draw };

/// Represents the state of a board, handles move validation and generation. Keeps track of various
/// things to allow for efficient use in the engine.
class Board {
   public:
    static constexpr int16_t SIZE = 64;

    static constexpr int16_t NEIGHBOURS_RADIUS = 2;

   private:
    bool whiteToMove_ = true;

    std::array<std::array<TileKind, SIZE>, SIZE> board_ = {};

    // Bitboards per axis for each player.
    std::array<uint64_t, SIZE> whiteBitboardX_{}, whiteBitboardY_{};
    std::array<uint64_t, SIZE> blackBitboardX_{}, blackBitboardY_{};
    std::array<uint64_t, 2 * SIZE - 1> whiteBitboardDiag_{}, blackBitboardDiag_{};

    /// Stores the number of alignments of each length for white
    std::array<uint32_t, GameRuleConstants::WINNING_ALIGNMENT_LENGTH + 1> alignmentCountWhite_ = {};
    /// Stores the number of alignments of each length for black
    std::array<uint32_t, GameRuleConstants::WINNING_ALIGNMENT_LENGTH + 1> alignmentCountBlack_ = {};

    /// Stores the history of moves made, used for undoing moves and possible moves generation.
    std::vector<Move> moveHistory_;

    /// Stores a Zobrist hash of the current board state, used for the transposition table in the
    /// engine. Updated incrementally after every move.
    uint64_t zobristHash_ = 0;

   public:
    [[nodiscard]] bool isWhiteToMove() const { return whiteToMove_; }
    [[nodiscard]] uint64_t hash() const { return zobristHash_; }

    [[nodiscard]] EndOfGameType endOfGame() const {
        if (alignmentCountWhite_[GameRuleConstants::WINNING_ALIGNMENT_LENGTH] > 0 ||
            alignmentCountBlack_[GameRuleConstants::WINNING_ALIGNMENT_LENGTH] > 0)
            return EndOfGameType::Win;
        return EndOfGameType::None;
    }

    [[nodiscard]] uint32_t countAlignments(const uint32_t length, const bool white) const {
        assert(length <= GameRuleConstants::WINNING_ALIGNMENT_LENGTH);
        if (white)
            return alignmentCountWhite_[length];
        else
            return alignmentCountBlack_[length];
    }

    /// Updates the board state by placing pieces according to the move. If the move is invalid,
    /// prints an error message and does not update the board.
    void makeMove(Move move, bool validate = true);
    /// Single piece move for the first move of the game.
    void makeMove(Coordinate coord1);

    /// Reverts the board state to before the last move was made.
    void undoMove();

    /// Returns a list of all valid moves for the current player.
    [[nodiscard]] std::vector<Move> possibleMoves() const;

    /// Prints a representation of the board to the console.
    void print() const;

    [[nodiscard]] TileKind get(const int16_t x, const int16_t y) const {
        assert(isInBounds(x, y));
        return board_[asIndex(x)][asIndex(y)];
    }

   private:
    [[nodiscard]] static size_t asIndex(const int16_t a) {
        assert(isInBounds(a));
        return a + SIZE / 2;
    }

    [[nodiscard]] bool isOccupied(const int16_t x, const int16_t y) const {
        assert(isInBounds(x, y));
        return get(x, y) != TileKind::Empty;
    }

    void set(TileKind kind, int16_t x, int16_t y);

    [[nodiscard]] Coordinate findAlignmentEnd(Coordinate origin, int16_t dx, int16_t dy,
                                              TileKind kind) const;

    /// Checks if the move is valid. If not, prints an error message.
    [[nodiscard]] bool validateMove(const Move& move) const;

    [[nodiscard]] static bool isInBounds(const int16_t a) { return a >= -SIZE / 2 && a < SIZE / 2; }

    [[nodiscard]] static bool isInBounds(const int16_t x, const int16_t y) {
        return isInBounds(x) && isInBounds(y);
    }

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

    [[nodiscard]] static consteval auto generateOffsets();
};
