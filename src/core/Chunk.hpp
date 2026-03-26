#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <span>

struct Coordinate {
    int16_t x, y;

    bool operator==(const Coordinate&) const = default;
};

template <>
struct std::hash<Coordinate> {
    size_t operator()(const Coordinate& c) const noexcept {
        return (static_cast<size_t>(c.x) << 16) | (static_cast<size_t>(c.y) & 0xFFFF);
    }
};

enum class TileKind : uint8_t { Empty, Black, White };

class Chunk {
   public:
    static constexpr size_t SIZE = 16;

   private:
    const Coordinate baseCoords_;

    std::array<std::array<TileKind, SIZE>, SIZE> tiles_{};

   public:
    explicit Chunk(const Coordinate baseCoords) : baseCoords_(baseCoords) {}

    [[nodiscard]] static Coordinate chunkBaseCoords(const Coordinate coords) {
        auto floorDiv = [](const int16_t a, const int16_t b) {
            if (a >= 0)
                return static_cast<int16_t>(a / b);
            else
                return static_cast<int16_t>((a - b + 1) / b);
        };
        constexpr int16_t S = SIZE;
        return {.x = static_cast<int16_t>(floorDiv(coords.x, S) * S),
                .y = static_cast<int16_t>(floorDiv(coords.y, S) * S)};
    }

    [[nodiscard]] std::span<const TileKind, SIZE * SIZE> getFlat() const {
        return std::span<const TileKind, SIZE * SIZE>(&tiles_[0][0], SIZE * SIZE);
    }

    [[nodiscard]] TileKind get(const Coordinate globalCoord) const {
        auto [x, y] = getRel(globalCoord);
        validateCoords(x, y);
        return tiles_[x][y];
    }

    [[nodiscard]] bool isEmpty(const Coordinate globalCoord) const {
        return get(globalCoord) == TileKind::Empty;
    }

    void set(const TileKind kind, const Coordinate globalCoord) {
        auto [x, y] = getRel(globalCoord);
        validateCoords(x, y);
        tiles_[x][y] = kind;
    }

    [[nodiscard]] Coordinate getGlobal(const int16_t x, const int16_t y) const {
        validateCoords(x, y);
        return Coordinate{.x = static_cast<int16_t>(baseCoords_.x + x),
                          .y = static_cast<int16_t>(baseCoords_.y + y)};
    }

   private:
    [[nodiscard]] Coordinate getRel(const Coordinate global) const {
        return {.x = static_cast<int16_t>(global.x - baseCoords_.x),
                .y = static_cast<int16_t>(global.y - baseCoords_.y)};
    }

    static void validateCoords(const int16_t x, const int16_t y) {
        assert(static_cast<size_t>(x) < SIZE && static_cast<size_t>(y) < SIZE);
    }
};
