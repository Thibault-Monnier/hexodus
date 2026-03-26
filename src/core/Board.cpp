#include "Board.hpp"

#include <algorithm>
#include <iostream>
#include <limits>

bool Board::isOccupied(const int16_t x, const int16_t y) const {
    const Coordinate coords{.x = x, .y = y};
    const Coordinate chunkBase = Chunk::chunkBaseCoords(coords);

    auto it = board_.find(chunkBase);
    return it != board_.end() && it->second.get(coords) != TileKind::Empty;
}

void Board::set(const TileKind kind, const int16_t x, const int16_t y) {
    const Coordinate coords{.x = x, .y = y};
    const Coordinate chunkBase = Chunk::chunkBaseCoords(coords);

    auto it = board_.find(chunkBase);
    if (it == board_.end()) {
        it = board_.emplace(chunkBase, chunkBase).first;
    }

    it->second.set(kind, coords);
}

void Board::print() const {
    if (board_.empty()) {
        std::cout << "Empty board\n";
        return;
    }

    int16_t maxX = std::numeric_limits<int16_t>::min(), maxY = maxX;
    int16_t minX = std::numeric_limits<int16_t>::max(), minY = minX;

    for (const auto& [chunkBase, chunk] : board_) {
        for (int16_t dy = 0; dy < static_cast<int16_t>(Chunk::SIZE); ++dy) {
            for (int16_t dx = 0; dx < static_cast<int16_t>(Chunk::SIZE); ++dx) {
                const Coordinate global{.x = static_cast<int16_t>(chunkBase.x + dx),
                                        .y = static_cast<int16_t>(chunkBase.y + dy)};
                if (chunk.get(global) != TileKind::Empty) {
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
