#pragma once

#include <unordered_map>
#include <vector>

#include "Chunk.hpp"
#include "Move.hpp"

enum class EndOfGameType : uint8_t { None, Win, Draw };

/// Represents the state of a board, and handles move validation and generation.
class Board {
    bool whiteToMove_ = true;
    std::unordered_map<Coordinate, Chunk> board_;

    /// Stores the start and end coordinates of every line of 2 or more consecutive aligned
    /// pieces of the same color, along with the coordinate of the piece that added the alignment.
    /// Used to efficiently check for end-of-game conditions and for position evaluation in the
    /// engine.
    std::vector<std::pair<std::pair<Coordinate, Coordinate>, Coordinate>> alignments_;

   public:
    [[nodiscard]] bool isOccupied(int16_t x, int16_t y) const;
    [[nodiscard]] bool isWhiteToMove() const { return whiteToMove_; }

    [[nodiscard]] EndOfGameType endOfGame() const;

    /// Updates the board state by placing pieces according to the move. If the move is invalid,
    /// prints an error message and does not update the board.
    void makeMove(Move move);
    /// Single piece move for the first move of the game.
    void makeMove(Coordinate coord1);

    /// Reverts the board state to before the move was made.
    void undoMove(Move move);

    /// Returns a list of all valid moves for the current player.
    [[nodiscard]] std::vector<Move> possibleMoves() const;

    /// Prints a representation of the board to the console.
    void print() const;

   private:
    void set(TileKind kind, int16_t x, int16_t y);

    /// Checks if the move is valid. If not, prints an error message.
    [[nodiscard]] bool validateMove(const Move& move) const;
};
