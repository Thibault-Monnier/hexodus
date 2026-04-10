#pragma once

#include <cstdint>

#include "Coordinate.hpp"

namespace HexCoordinates {

/// Calculates the distance between two hexagonal coordinates given their axial differences dx and
/// dy. The distance is the minimum number of steps required to move from one coordinate to the
/// other while following the hexagonal grid.
[[nodiscard]] constexpr int16_t hexDistance(const int16_t dx, const int16_t dy) {
    auto abs = [](const int x) { return x < 0 ? -x : x; };
    return static_cast<int16_t>((abs(dx) + abs(dy) + abs(dx + dy)) / 2);
}

/// Calculates the distance between two hexagonal coordinates given. The distance is the minimum
/// number of steps required to move from one coordinate to the other while following the hexagonal
/// grid.
[[nodiscard]] constexpr int16_t hexDistance(const Coordinate coord1, const Coordinate coord2) {
    return hexDistance(static_cast<int16_t>(coord1.x - coord2.x),
                       static_cast<int16_t>(coord1.y - coord2.y));
}

/// High-performance check whether two coordinates are adjacent on a hexagonal grid.
[[nodiscard]] constexpr bool areAdjacent(const Coordinate coord1, const Coordinate coord2) {
    const auto [dx, dy] = coord1 - coord2;
    // Per-axis check, all summed
    return (dx * dx + dy * dy + dx * dy) == 1;
}

}  // namespace HexCoordinates
