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
    const Coordinate coord{x, y};

    const TileKind prevKind = get(coord);

    board_[asIndex(coord.x)][asIndex(coord.y)] = kind;

    const bool set = kind != TileKind::Empty;
    updateCandidates(coord, set);
    updateAlignments(coord, set ? kind : prevKind, set);
}

void Board::updateAlignments(const Coordinate coord, const TileKind kind, const bool set) {
    constexpr std::array<Coordinate, 3> DIRECTIONS = {Coordinate{1, 0}, Coordinate{0, 1},
                                                      Coordinate{-1, 1}};
    auto& alignmentCount = whiteToMove_ ? alignmentCountWhite_ : alignmentCountBlack_;

    const int sign = set ? 1 : -1;
    for (const Coordinate dir : DIRECTIONS) {
        const uint32_t count1 = findAlignmentLength(coord, dir, kind);
        const uint32_t count2 = findAlignmentLength(coord, -dir, kind);

        const uint32_t totalCount =
            std::min(count1 + count2 + 1,
                     static_cast<uint32_t>(GameRuleConstants::WINNING_ALIGNMENT_LENGTH));

        if (totalCount >= 2) alignmentCount[totalCount] += sign;

        if (count1 >= 2) alignmentCount[count1] -= sign;
        if (count2 >= 2) alignmentCount[count2] -= sign;
    }
}

uint32_t Board::findAlignmentLength(const Coordinate start, const Coordinate dir,
                                    const TileKind kind) const {
    Coordinate coord = start;

    uint32_t count = 0;
    for (; count <= GameRuleConstants::WINNING_ALIGNMENT_LENGTH; ++count) {
        coord += dir;
        if (get(coord) != kind) break;
    }

    return count;
}

void Board::updateCandidates(const Coordinate coord, const bool set) {
    constexpr auto OFFSETS = generateOffsets();

    {
        const size_t index = asIndex(coord.x) * SIZE + asIndex(coord.y);
        if (set) {
            candidateSet_.erase(index);
        } else {
            if (candidateCount_[index] > 0) {
                candidateSet_.insert(coord, index);
            }
        }
    }

    for (const Coordinate offset : OFFSETS) {
        const Coordinate newCoord = coord + offset;

        const size_t index = asIndex(newCoord.x) * SIZE + asIndex(newCoord.y);
        candidateCount_[index] += set ? 1 : -1;
        if (set && candidateCount_[index] == 1 && !isOccupied(newCoord)) {
            candidateSet_.insert(newCoord, index);
        }
        if (candidateCount_[index] == 0) {
            candidateSet_.erase(index);
        }
    }
}

bool Board::validateMove(const Move& move) const {
    if (!isInBounds(move.coord1)) {
        std::cerr << "Invalid move: coordinate (" << move.coord1.x << ", " << move.coord1.y
                  << ") is out of bounds.\n";
        return false;
    }

    if (isOccupied(move.coord1)) {
        std::cerr << "Invalid move: coordinate (" << move.coord1.x << ", " << move.coord1.y
                  << ") is already occupied.\n";
        return false;
    }

    if (isOccupied(move.coord2)) {
        std::cerr << "Invalid move: coordinate (" << move.coord2.x << ", " << move.coord2.y
                  << ") is already occupied.\n";
        return false;
    }

    return true;
}

__attribute__((no_sanitize("unsigned-integer-overflow"))) void Board::updateHash(const int16_t x,
                                                                                 const int16_t y,
                                                                                 TileKind kind) {
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
