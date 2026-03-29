#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <span>

#include "Coordinate.hpp"

enum class TileKind : uint8_t { Empty, Black, White };

class Chunk {
   public:
    static constexpr size_t SIZE = 7;

   private:
    const Coordinate baseCoords_;

    std::array<std::array<TileKind, SIZE>, SIZE> tiles_{};

   public:
    explicit Chunk(const Coordinate baseCoords) : baseCoords_(baseCoords) {}

    [[nodiscard]] static Coordinate chunkBaseCoords(Coordinate coords);

    [[nodiscard]] std::span<const TileKind, SIZE * SIZE> getFlat() const {
        return std::span<const TileKind, SIZE * SIZE>(&tiles_[0][0], SIZE * SIZE);
    }

    [[nodiscard]] TileKind get(Coordinate globalCoord) const;

    [[nodiscard]] bool isEmpty(const Coordinate globalCoord) const {
        return get(globalCoord) == TileKind::Empty;
    }

    void set(TileKind kind, Coordinate globalCoord);

    [[nodiscard]] Coordinate getGlobal(const int16_t x, const int16_t y) const {
        validateCoords(x, y);
        return Coordinate{static_cast<int16_t>(baseCoords_.x + x),
                          static_cast<int16_t>(baseCoords_.y + y)};
    }

   private:
    [[nodiscard]] Coordinate getRel(const Coordinate global) const {
        return {static_cast<int16_t>(global.x - baseCoords_.x),
                static_cast<int16_t>(global.y - baseCoords_.y)};
    }

    static void validateCoords([[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y) {
        assert(static_cast<size_t>(x) < SIZE && static_cast<size_t>(y) < SIZE);
    }
};
