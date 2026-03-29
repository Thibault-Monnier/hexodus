#include "Chunk.hpp"

Coordinate Chunk::chunkBaseCoords(const Coordinate coords) {
    auto floorDiv = [](const int16_t a, const int16_t b) {
        if (a >= 0)
            return static_cast<int16_t>(a / b);
        else
            return static_cast<int16_t>((a - b + 1) / b);
    };

    constexpr int16_t S = SIZE;
    return {static_cast<int16_t>(floorDiv(coords.x, S) * S),
            static_cast<int16_t>(floorDiv(coords.y, S) * S)};
}

TileKind Chunk::get(const Coordinate globalCoord) const {
    auto [x, y] = getRel(globalCoord);
    validateCoords(x, y);
    return tiles_[x][y];
}

void Chunk::set(const TileKind kind, const Coordinate globalCoord) {
    auto [x, y] = getRel(globalCoord);
    validateCoords(x, y);
    tiles_[x][y] = kind;
}
