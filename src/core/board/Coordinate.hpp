#pragma once

#include <cstdint>

struct Coordinate {
    int16_t x = 0, y = 0;

    Coordinate() = default;
    constexpr Coordinate(const int16_t x, const int16_t y) : x(x), y(y) {}

    bool operator==(const Coordinate&) const = default;

    Coordinate operator+(const Coordinate& other) const {
        return {static_cast<int16_t>(x + other.x), static_cast<int16_t>(y + other.y)};
    }

    Coordinate operator-(const Coordinate& other) const {
        return {static_cast<int16_t>(x - other.x), static_cast<int16_t>(y - other.y)};
    }

    Coordinate operator-() const { return Coordinate() - *this; }

    Coordinate& operator+=(const Coordinate& other) { return *this = *this + other; }

    Coordinate& operator-=(const Coordinate& other) { return *this = *this - other; }
};
