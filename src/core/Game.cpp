#include "Game.hpp"

#include <iostream>

void Game::makeMove(const Coordinate coord1, const Coordinate coord2) {
    const TileKind tileKind = whiteToMove_ ? TileKind::White : TileKind::Black;

    const Move move(coord1, coord2);
    if (!validateMove(move)) return;

    board_.set(tileKind, coord1.x, coord1.y);
    board_.set(tileKind, coord2.x, coord2.y);
    whiteToMove_ = !whiteToMove_;
}

void Game::makeMove(const Coordinate coord1) {
    const TileKind tileKind = whiteToMove_ ? TileKind::White : TileKind::Black;
    board_.set(tileKind, coord1.x, coord1.y);
    whiteToMove_ = !whiteToMove_;
}

bool Game::validateMove(const Move& move) const {
    if (board_.isOccupied(move.getCoord1().x, move.getCoord1().y)) {
        std::cerr << "Invalid move: coordinate (" << move.getCoord1().x << ", "
                  << move.getCoord1().y << ") is already occupied.\n";
        return false;
    }

    if (board_.isOccupied(move.getCoord2().x, move.getCoord2().y)) {
        std::cerr << "Invalid move: coordinate (" << move.getCoord2().x << ", "
                  << move.getCoord2().y << ") is already occupied.\n";
        return false;
    }

    return true;
}
