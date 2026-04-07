#include "Board.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <ranges>

#include "HexCoordinates.hpp"
#include "Move.hpp"
#include "core/GameRuleConstants.hpp"

void Board::makeMove(const Move move, const bool validate) {
    const Coordinate coord1 = move.coord1, coord2 = move.coord2;

    const TileKind tileKind = whiteToMove_ ? TileKind::White : TileKind::Black;

    if (validate && !validateMove(move)) return;
    assert(validateMove(move));

    set(tileKind, coord1.x, coord1.y);
    set(tileKind, coord2.x, coord2.y);

    updateHash(coord1.x, coord1.y, tileKind);
    updateHash(coord2.x, coord2.y, tileKind);

    whiteToMove_ = !whiteToMove_;
    moveHistory_.push_back(move);
}

void Board::makeMove(const Coordinate coord1) {
    const TileKind tileKind = whiteToMove_ ? TileKind::White : TileKind::Black;
    set(tileKind, coord1.x, coord1.y);

    updateHash(coord1.x, coord1.y, tileKind);

    whiteToMove_ = !whiteToMove_;
    moveHistory_.push_back(Move(coord1, coord1));
}

void Board::undoMove() {
    if (moveHistory_.size() < 2) {
        std::cerr << "No moves to undo.\n";
        return;
    }

    const Move move = popLastMove();

    whiteToMove_ = !whiteToMove_;

    // Undo pieces
    const Coordinate coord1 = move.coord1, coord2 = move.coord2;
    set(TileKind::Empty, coord1.x, coord1.y);
    set(TileKind::Empty, coord2.x, coord2.y);

    // Undo hash
    const TileKind tileKind = whiteToMove_ ? TileKind::White : TileKind::Black;
    updateHash(coord1.x, coord1.y, tileKind);
    updateHash(coord2.x, coord2.y, tileKind);
}

consteval auto Board::generateOffsets() {
    constexpr size_t COUNT = 3ull * NEIGHBOURS_RADIUS * (NEIGHBOURS_RADIUS + 1);
    std::array<Coordinate, COUNT> offsets{};
    size_t index = 0;
    for (int16_t dx = -NEIGHBOURS_RADIUS; dx <= NEIGHBOURS_RADIUS; ++dx) {
        for (int16_t dy = -NEIGHBOURS_RADIUS; dy <= NEIGHBOURS_RADIUS; ++dy) {
            if (dx == 0 && dy == 0) continue;
            if (HexCoordinates::hexDistance(dx, dy) > NEIGHBOURS_RADIUS) continue;

            offsets[index++] = Coordinate{dx, dy};
        }
    }
    return offsets;
}

void Board::generatePossibleMoves(std::vector<Move>& outMoves) const {
    constexpr auto OFFSETS = generateOffsets();

    const auto [coord1, coord2] = lastMove();
    const auto [coord3, coord4] = moveHistory_.size() >= 2 ? beforeLastMove() : lastMove();

    std::vector<Coordinate> tiles;
    tiles.reserve(OFFSETS.size() * 4);

    std::bitset<static_cast<size_t>(SIZE * SIZE)> seen;

    // Push each offset around the 4 coords
    for (const Coordinate base : {coord1, coord2, coord3, coord4}) {
        for (const Coordinate offset : OFFSETS) {
            const Coordinate coord = base + offset;

            if (isOccupied(coord.x, coord.y)) continue;

            const size_t index = asIndex(coord.x) * SIZE + asIndex(coord.y);
            if (!seen.test(index)) {
                seen.set(index);
                tiles.push_back(coord);
            }
        }
    }

    assert(outMoves.empty());
    outMoves.reserve(tiles.size() * (tiles.size() - 1));
    for (size_t i = 0; i < tiles.size(); ++i) {
        const Coordinate tile1 = tiles[i];
        for (size_t j = i + 1; j < tiles.size(); ++j) {
            const Coordinate tile2 = tiles[j];
            if (tile1 == tile2) continue;

            outMoves.emplace_back(tile1, tile2);
        }
    }
}

void Board::print() const {
    int16_t maxX = std::numeric_limits<int16_t>::min(), maxY = maxX;
    int16_t minX = std::numeric_limits<int16_t>::max(), minY = minX;

    for (int16_t x = -SIZE / 2; x < SIZE / 2; ++x) {
        for (int16_t y = -SIZE / 2; y < SIZE / 2; ++y) {
            if (!isOccupied(x, y)) continue;

            maxX = std::max(maxX, x);
            minX = std::min(minX, x);
            maxY = std::max(maxY, y);
            minY = std::min(minY, y);
        }
    }

    constexpr std::string_view COLOR_EMPTY = "\033[1;90";
    constexpr std::string_view COLOR_ORIGIN = "\033[1;93";
    constexpr std::string_view COLOR_WHITE = "\033[1;33";
    constexpr std::string_view COLOR_BLACK = "\033[1;94";
    constexpr std::string_view LAST_MOVE_SUFFIX = ";4";
    constexpr std::string_view COLOR_AXIS = "\033[3m";
    constexpr std::string_view RESET = "\033[0m";

    for (int16_t y = maxY; y >= minY; --y) {
        std::cout << COLOR_AXIS << std::setw(3) << std::right << y << RESET << ' ';

        for (int16_t i = 0; i < y - minY; ++i) std::cout << " ";

        for (int16_t x = minX; x <= maxX; ++x) {
            const TileKind kind = get(x, y);
            std::string_view c = "·";
            if (kind == TileKind::White)
                c = "X";
            else if (kind == TileKind::Black)
                c = "O";

            std::string color = std::string(COLOR_EMPTY);
            if (x == 0 && y == 0)
                color = COLOR_ORIGIN;
            else if (kind == TileKind::White)
                color = COLOR_WHITE;
            else if (kind == TileKind::Black)
                color = COLOR_BLACK;

            if (moveHistory_.size() >= 1 &&
                ((x == lastMove().coord1.x && y == lastMove().coord1.y) ||
                 (x == lastMove().coord2.x && y == lastMove().coord2.y))) {
                color += LAST_MOVE_SUFFIX;
            }
            color += 'm';

            std::cout << color << c << RESET << ' ';
        }

        std::cout << '\n';
    }

    std::cout << "   ";
    for (int16_t x = minX; x <= maxX; ++x) {
        std::cout << COLOR_AXIS << std::left << std::setw(2) << std::abs(x % 10) << RESET;
    }
    std::cout << '\n';

    // Print some stats
    std::cout << "Stats:\n";
    std::cout << "  White alignments: ";
    for (size_t len = 2; len <= GameRuleConstants::WINNING_ALIGNMENT_LENGTH; ++len) {
        std::cout << len << ": " << alignmentCountWhite_[len] << "  ";
    }
    std::cout << "\n  Black alignments: ";
    for (size_t len = 2; len <= GameRuleConstants::WINNING_ALIGNMENT_LENGTH; ++len) {
        std::cout << len << ": " << alignmentCountBlack_[len] << "  ";
    }
    std::cout << '\n';
}

void Board::set(const TileKind kind, const int16_t x, const int16_t y) {
    const Coordinate coords{x, y};

    const size_t idxX = asIndex(coords.x);
    const size_t idxY = asIndex(coords.y);

    board_[idxX][idxY] = kind;

    auto updateAlignments = [this](const auto& bitboard, const size_t idx,
                                   const bool clearing = false) {
        uint64_t xBits = bitboard[idx];

        int c1 = std::popcount(xBits);
        xBits &= xBits >> 1;
        int c2 = std::popcount(xBits);

        for (size_t len = 2; len <= GameRuleConstants::WINNING_ALIGNMENT_LENGTH + 1; ++len) {
            xBits &= xBits >> 1;

            const int c3 = std::popcount(xBits);

            if (len >= 2) {
                // The amount of alignments of length = len
                const int amount = c1 - 2 * c2 + c3;
                if (clearing) {
                    if (whiteToMove_)
                        alignmentCountWhite_[len - 1] -= amount;
                    else
                        alignmentCountBlack_[len - 1] -= amount;
                } else {
                    if (whiteToMove_)
                        alignmentCountWhite_[len - 1] += amount;
                    else
                        alignmentCountBlack_[len - 1] += amount;
                }
            }

            c1 = c2;
            c2 = c3;
        }
    };

    auto& bitboardX = whiteToMove_ ? whiteBitboardX_ : blackBitboardX_;
    auto& bitboardY = whiteToMove_ ? whiteBitboardY_ : blackBitboardY_;
    auto& bitboardDiag = whiteToMove_ ? whiteBitboardDiag_ : blackBitboardDiag_;

    updateAlignments(bitboardX, idxY, true);
    updateAlignments(bitboardY, idxX, true);
    updateAlignments(bitboardDiag, idxX + idxY, true);

    const uint64_t maskX = 1ull << idxX;
    const uint64_t maskY = 1ull << idxY;
    const uint64_t maskDiag = 1ull << idxY;

    if (kind == TileKind::Empty) {
        bitboardX[idxY] &= ~maskX;
        bitboardY[idxX] &= ~maskY;
        bitboardDiag[idxX + idxY] &= ~maskDiag;
    } else {
        bitboardX[idxY] |= maskX;
        bitboardY[idxX] |= maskY;
        bitboardDiag[idxX + idxY] |= maskDiag;
    }

    updateAlignments(bitboardX, idxY);
    updateAlignments(bitboardY, idxX);
    updateAlignments(bitboardDiag, idxX + idxY);
}

__attribute__((always_inline)) Coordinate Board::findAlignmentEnd(const Coordinate origin,
                                                                  const int16_t dx,
                                                                  const int16_t dy,
                                                                  const TileKind kind) const {
    Coordinate last = origin;

    for (uint32_t i = 1; i < GameRuleConstants::WINNING_ALIGNMENT_LENGTH; ++i) {
        const auto x = static_cast<int16_t>(origin.x + i * dx);
        const auto y = static_cast<int16_t>(origin.y + i * dy);
        if (get(x, y) != kind) break;

        last.x = x;
        last.y = y;
    }

    return last;
}

bool Board::validateMove(const Move& move) const {
    if (!isInBounds(move.coord1.x, move.coord1.y)) {
        std::cerr << "Invalid move: coordinate (" << move.coord1.x << ", " << move.coord1.y
                  << ") is out of bounds.\n";
        return false;
    }

    if (isOccupied(move.coord1.x, move.coord1.y)) {
        std::cerr << "Invalid move: coordinate (" << move.coord1.x << ", " << move.coord1.y
                  << ") is already occupied.\n";
        return false;
    }

    if (isOccupied(move.coord2.x, move.coord2.y)) {
        std::cerr << "Invalid move: coordinate (" << move.coord2.x << ", " << move.coord2.y
                  << ") is already occupied.\n";
        return false;
    }

    return true;
}

void Board::updateHash(const int16_t x, const int16_t y, TileKind kind) {
    uint64_t seed = static_cast<uint64_t>(static_cast<uint16_t>(x)) |
                    static_cast<uint64_t>(static_cast<uint16_t>(y)) << 16 |
                    static_cast<uint64_t>(static_cast<uint16_t>(kind)) << 32;
    seed ^= seed >> 33;
    seed *= 0xff51afd7ed558ccdULL;
    seed ^= seed >> 33;
    seed *= 0xc4ceb9fe1a85ec53ULL;
    seed ^= seed >> 33;

    zobristHash_ ^= seed;
}
