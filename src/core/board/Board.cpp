#include "Board.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <ranges>
#include <unordered_set>
#include <utility>

#include "Move.hpp"
#include "core/GameRuleConstants.hpp"

bool Board::isOccupied(const int16_t x, const int16_t y) const {
    const Coordinate coords{x, y};
    const Coordinate chunkBase = Chunk::chunkBaseCoords(coords);

    const auto it = board_.find(chunkBase);
    return it != board_.end() && it->second.get(coords) != TileKind::Empty;
}

EndOfGameType Board::endOfGame() const {
    for (const auto [start, end] : alignments_ | std::views::keys) {
        const uint16_t length = std::max(std::abs(end.x - start.x), std::abs(end.y - start.y)) + 1;
        if (length >= GameRuleConstants::WINNING_ALIGNMENT_LENGTH) return EndOfGameType::Win;
    }

    return EndOfGameType::None;
}

void Board::makeMove(const Move move) {
    const Coordinate coord1 = move.coord1, coord2 = move.coord2;

    const TileKind tileKind = whiteToMove_ ? TileKind::White : TileKind::Black;

    if (!validateMove(move)) return;

    set(tileKind, coord1.x, coord1.y);
    set(tileKind, coord2.x, coord2.y);
    whiteToMove_ = !whiteToMove_;
    moveHistory_.push_back(move);
}

void Board::makeMove(const Coordinate coord1) {
    const TileKind tileKind = whiteToMove_ ? TileKind::White : TileKind::Black;
    set(tileKind, coord1.x, coord1.y);
    whiteToMove_ = !whiteToMove_;
    moveHistory_.push_back(Move(coord1, coord1));
}

void Board::undoMove() {
    if (moveHistory_.size() < 2) {
        std::cerr << "No moves to undo.\n";
        return;
    }

    counter++;

    const Move move = popLastMove();

    // Undo pieces
    const Coordinate coord1 = move.coord1, coord2 = move.coord2;
    set(TileKind::Empty, coord1.x, coord1.y);
    set(TileKind::Empty, coord2.x, coord2.y);

    // Undo alignments
    std::erase_if(alignments_, [move](const auto& alignment) {
        const Coordinate addedCoord = alignment.second;
        return addedCoord == move.coord1 || addedCoord == move.coord2;
    });

    whiteToMove_ = !whiteToMove_;
}

std::vector<Move> Board::possibleMoves() const {
    // Half the offsets, for the rest just take the opposite of these
    // Radius of 2 to avoid too many possible moves -> combinatorial explosion
    constexpr std::array<Coordinate, 9> OFFSETS = {
        Coordinate{1, 0}, {0, 1}, {-1, 1}, {2, 0}, {1, 1}, {0, 2}, {-1, 2}, {-2, 2}, {-2, 1}};

    const auto [coord1, coord2] = lastMove();
    const auto [coord3, coord4] = moveHistory_.size() >= 2 ? beforeLastMove() : lastMove();

    std::unordered_set<Coordinate> tiles;
    // Push each offset around the 4 coords
    for (const Coordinate base : {coord1, coord2, coord3, coord4}) {
        for (const Coordinate offset : OFFSETS) {
            const Coordinate coord = base + offset;
            const Coordinate coordOther = base - offset;
            if (!isOccupied(coord.x, coord.y)) tiles.insert(coord);
            if (!isOccupied(coordOther.x, coordOther.y)) tiles.insert(coordOther);
        }
    }

    std::vector<Move> moves;
    moves.reserve(tiles.size() * (tiles.size() - 1));
    for (const Coordinate tile1 : tiles) {
        for (const Coordinate tile2 : tiles) {
            if (tile1 == tile2) continue;
            moves.emplace_back(tile1, tile2);
        }
    }

    return moves;
}

void Board::print() const {
    if (board_.empty()) {
        std::cout << "Empty board\n";
        return;
    }

    int16_t maxX = std::numeric_limits<int16_t>::min(), maxY = maxX;
    int16_t minX = std::numeric_limits<int16_t>::max(), minY = minX;

    for (const auto& chunk : board_ | std::views::values) {
        for (int16_t dx = 0; dx < static_cast<int16_t>(Chunk::SIZE); ++dx) {
            for (int16_t dy = 0; dy < static_cast<int16_t>(Chunk::SIZE); ++dy) {
                const Coordinate global = chunk.getGlobal(dx, dy);
                if (!chunk.isEmpty(global)) {
                    maxX = std::max(maxX, global.x);
                    minX = std::min(minX, global.x);
                    maxY = std::max(maxY, global.y);
                    minY = std::min(minY, global.y);
                }
            }
        }
    }

    for (int16_t y = maxY; y >= minY; --y) {
        for (int16_t i = 0; i < y - minY; ++i) std::cout << ' ';

        for (int16_t x = minX; x <= maxX; ++x) {
            const Coordinate coords{x, y};
            const Coordinate chunkBase = Chunk::chunkBaseCoords(coords);
            auto it = board_.find(chunkBase);
            const TileKind kind = (it == board_.end()) ? TileKind::Empty : it->second.get(coords);

            char c = '.';
            if (kind == TileKind::White)
                c = 'X';
            else if (kind == TileKind::Black)
                c = 'O';
            std::cout << c << ' ';
        }
        std::cout << '\n';
    }
}

void Board::set(const TileKind kind, const int16_t x, const int16_t y) {
    const Coordinate coords{x, y};
    const Coordinate chunkBase = Chunk::chunkBaseCoords(coords);

    auto it = board_.find(chunkBase);
    if (it == board_.end()) {
        it = board_.emplace(chunkBase, chunkBase).first;
    }

    it->second.set(kind, coords);

    if (kind == TileKind::Empty) return;

    // Update alignments
    // Find new alignments in all 6 directions
    constexpr std::array<std::pair<int16_t, int16_t>, 3> DIRECTIONS = {{{1, 0}, {0, 1}, {-1, 1}}};

    for (const auto& [dx, dy] : DIRECTIONS) {
        const Coordinate start = findAlignmentEnd(coords, dx, dy, kind);
        const Coordinate end = findAlignmentEnd(coords, -dx, -dy, kind);

        // Add alignment if length >= 2
        const uint16_t length = std::max(std::abs(end.x - start.x), std::abs(end.y - start.y)) + 1;
        if (length >= 2) {
            alignments_.emplace_back(std::pair{start, end}, coords);
        }
    }
}

Coordinate Board::findAlignmentEnd(const Coordinate origin, const int16_t dx, const int16_t dy,
                                   const TileKind kind) {
    Coordinate last = origin;
    for (int16_t i = 1; std::cmp_less_equal(i, GameRuleConstants::WINNING_ALIGNMENT_LENGTH); ++i) {
        // TODO: If we later auto generate new chunks when there's a tile near the edge, we
        //  might not need to check for out of bounds here.
        const Coordinate check{static_cast<int16_t>(origin.x + i * dx),
                               static_cast<int16_t>(origin.y + i * dy)};
        const Coordinate checkChunkBase = Chunk::chunkBaseCoords(check);
        const auto checkIt = board_.find(checkChunkBase);
        if (checkIt == board_.end() || checkIt->second.get(check) != kind) break;
        last = check;
    }

    return last;
}

bool Board::validateMove(const Move& move) const {
    if (isOccupied(move.coord1.x, move.coord1.y)) {
        std::cerr << "Invalid move: coordinate (" << move.coord1.x << ", " << move.coord1.y
                  << ") is already occupied.\n";
        return false;
    }

    if (isOccupied(move.coord2.x, move.coord2.y)) {
        std::cerr << "Invalid move: coordinate (" << move.coord2.x << ", " << move.coord2.y
                  << ") is already occupied.\n";
        return false;
    }

    return true;
}
