#pragma once

struct Move {
    Coordinate coord1;
    Coordinate coord2;

    bool operator==(const Move& move) const = default;
};
