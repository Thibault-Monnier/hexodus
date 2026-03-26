#include <iostream>

#include "core/Board.hpp"
#include "interface/CommandProcessor.hpp"

void dummyTest() {
    Board board;
    board.set(TileKind::Black, 0, 0);   // Center
    board.set(TileKind::White, 1, 0);   // Right
    board.set(TileKind::Black, 0, 1);   // Top-Right
    board.set(TileKind::White, -1, 1);  // Top-Left
    board.set(TileKind::Black, -1, 0);  // Left
    board.set(TileKind::White, 0, -1);  // Bottom-Left
    board.set(TileKind::Black, 1, -1);  // Bottom-Right
    board.set(TileKind::White, 2, 0);   // Right-Right
    board.set(TileKind::Black, 0, -2);  // Bottom-Bottom-Left
    board.set(TileKind::White, 1, -2);  // Bottom-Bottom-Right

    std::cout << "Board Layout:\n";
    board.print();
}

int main() {
    while (true) {
        const Command command = CommandProcessor::waitForCommand();

        switch (command.getKind()) {
            case Command::Kind::Ping:
                std::cout << "Hi!\n";
                break;
            case Command::Kind::Test:
                dummyTest();
                break;
            case Command::Kind::Quit:
                std::cout << "Quitting...\n";
                return 0;
        }
    }
}
