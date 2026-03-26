#include "Board.hpp"

#include <algorithm>
#include <iostream>
#include <limits>

void Board::set(const TileKind kind, const int x, const int y) {
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

    int maxX = std::numeric_limits<int>::min(), maxY = maxX;
    int minX = std::numeric_limits<int>::max(), minY = minX;

    for (const auto& [chunkBase, chunk] : board_) {
        for (int dy = 0; dy < static_cast<int>(Chunk::SIZE); ++dy) {
            for (int dx = 0; dx < static_cast<int>(Chunk::SIZE); ++dx) {
                const Coordinate global{.x = chunkBase.x + dx, .y = chunkBase.y + dy};
                if (chunk.get(global) != TileKind::Empty) {
                    maxX = std::max(maxX, global.x);
                    minX = std::min(minX, global.x);
                    maxY = std::max(maxY, global.y);
                    minY = std::min(minY, global.y);
                }
            }
        }
    }

    for (int y = maxY; y >= minY; --y) {
        for (int i = 0; i < y - minY; ++i) std::cout << ' ';
        for (int x = minX; x <= maxX; ++x) {
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
