#pragma once

#include <cstdint>
#include <unordered_map>

struct Coordinate {
    int16_t x, y;

    Coordinate() = default;
    constexpr Coordinate(const int16_t x, const int16_t y) : x(x), y(y) {}

    bool operator==(const Coordinate&) const = default;

    Coordinate operator+(const Coordinate& other) const {
        return {static_cast<int16_t>(x + other.x), static_cast<int16_t>(y + other.y)};
    }

    Coordinate operator-(const Coordinate& other) const {
        return {static_cast<int16_t>(x - other.x), static_cast<int16_t>(y - other.y)};
    }
};

template <>
struct std::hash<Coordinate> {
    size_t operator()(const Coordinate& c) const noexcept {
        return (static_cast<size_t>(c.x) << 16) | (static_cast<size_t>(c.y) & 0xFFFF);
    }
};
