#include "Board.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <ranges>
#include <utility>

#include "GameRuleConstants.hpp"
#include "Move.hpp"

bool Board::isOccupied(const int16_t x, const int16_t y) const {
    const Coordinate coords{.x = x, .y = y};
    const Coordinate chunkBase = Chunk::chunkBaseCoords(coords);

    const auto it = board_.find(chunkBase);
    return it != board_.end() && it->second.get(coords) != TileKind::Empty;
}

EndOfGameType Board::endOfGame() const {
    for (const auto [startCoord, endCoord] : alignments_) {
        const uint16_t length =
            std::max(std::abs(endCoord.x - startCoord.x), std::abs(endCoord.y - startCoord.y)) + 1;
        if (length >= GameRuleConstants::WINNING_ALIGNMENT_LENGTH) return EndOfGameType::Win;
    }

    return EndOfGameType::None;
}

void Board::makeMove(const Move move) {
    const Coordinate coord1 = move.getCoord1(), coord2 = move.getCoord2();

    const TileKind tileKind = whiteToMove_ ? TileKind::White : TileKind::Black;

    if (!validateMove(move)) return;

    set(tileKind, coord1.x, coord1.y);
    set(tileKind, coord2.x, coord2.y);
    whiteToMove_ = !whiteToMove_;
}

void Board::makeMove(const Coordinate coord1) {
    const TileKind tileKind = whiteToMove_ ? TileKind::White : TileKind::Black;
    set(tileKind, coord1.x, coord1.y);
    whiteToMove_ = !whiteToMove_;
}

void Board::undoMove(const Move move) {
    const Coordinate coord1 = move.getCoord1(), coord2 = move.getCoord2();
    set(TileKind::Empty, coord1.x, coord1.y);
    set(TileKind::Empty, coord2.x, coord2.y);
    whiteToMove_ = !whiteToMove_;
}

std::vector<Move> Board::possibleMoves() const {
    constexpr int16_t S = Chunk::SIZE;

    std::vector<Coordinate> emptyTiles;
    for (const auto& [chunkBase, chunk] : board_) {
        auto tiles = chunk.getFlat();
        for (int i = 0; i < S * S; ++i) {
            if (tiles[i] != TileKind::Empty) continue;

            emptyTiles.push_back({static_cast<int16_t>(chunkBase.x + i / S),
                                  static_cast<int16_t>(chunkBase.y + i % S)});
        }
    }

    std::vector<Move> moves;
    moves.reserve(board_.size() * Chunk::SIZE * Chunk::SIZE * Chunk::SIZE * Chunk::SIZE);

    for (const Coordinate emptyTile1 : emptyTiles) {
        for (const Coordinate emptyTile2 : emptyTiles) {
            if (emptyTile1 == emptyTile2) continue;
            moves.emplace_back(emptyTile1, emptyTile2);
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
            const Coordinate coords{.x = x, .y = y};
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
    const Coordinate coords{.x = x, .y = y};
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
        Coordinate start = coords;
        Coordinate end = coords;

        // Check in both directions until a different tile kind is found
        for (int16_t i = 1; std::cmp_less_equal(i, GameRuleConstants::WINNING_ALIGNMENT_LENGTH);
             ++i) {
            // TODO: If we later auto generate new chunks when there's a tile near the edge, we
            //  might not need to check for out of bounds here.
            {
                const Coordinate check{.x = static_cast<int16_t>(coords.x - i * dx),
                                       .y = static_cast<int16_t>(coords.y - i * dy)};
                const Coordinate checkChunkBase = Chunk::chunkBaseCoords(check);
                const auto checkIt = board_.find(checkChunkBase);
                if (checkIt == board_.end() || checkIt->second.get(check) != kind) break;
                start = check;
            }
            {
                const Coordinate check{.x = static_cast<int16_t>(coords.x + i * dx),
                                       .y = static_cast<int16_t>(coords.y + i * dy)};
                const Coordinate checkChunkBase = Chunk::chunkBaseCoords(check);
                const auto checkIt = board_.find(checkChunkBase);
                if (checkIt == board_.end() || checkIt->second.get(check) != kind) break;
                end = check;
            }
        }

        // Add alignment if length >= 2
        const uint16_t length = std::max(std::abs(end.x - start.x), std::abs(end.y - start.y)) + 1;
        if (length >= 2) {
            alignments_.emplace_back(start, end);
        }
    }
}

bool Board::validateMove(const Move& move) const {
    if (isOccupied(move.getCoord1().x, move.getCoord1().y)) {
        std::cerr << "Invalid move: coordinate (" << move.getCoord1().x << ", "
                  << move.getCoord1().y << ") is already occupied.\n";
        return false;
    }

    if (isOccupied(move.getCoord2().x, move.getCoord2().y)) {
        std::cerr << "Invalid move: coordinate (" << move.getCoord2().x << ", "
                  << move.getCoord2().y << ") is already occupied.\n";
        return false;
    }

    return true;
}
