#include <iostream>

#include "core/Game.hpp"
#include "core/board/Board.hpp"
#include "interface/CommandProcessor.hpp"

int main() {
    Game game;

    while (true) {
        const std::unique_ptr<Command> commandPtr = CommandProcessor::waitForCommand();
        const Command& command = *commandPtr;

        switch (command.getKind()) {
            case Command::Kind::Print:
                std::cout << "Current game state:\n";
                game.getBoard().print();
                break;
            case Command::Kind::Move: {
                const auto& moveCommand = static_cast<const MoveCommand&>(command);
                game.makeMove(moveCommand.getCoord1(), moveCommand.getCoord2());
                break;
            }
            case Command::Kind::MoveRequest: {
                const Move bestMove = game.playBestMove(3);
                std::cout << "Best move: (" << bestMove.coord1.x << ", " << bestMove.coord1.y
                          << ") and (" << bestMove.coord2.x << ", " << bestMove.coord2.y << ")\n";
                break;
            }
            case Command::Kind::Analyse: {
                const Move bestMove = game.findBestMove(3);
                std::cout << "Best move: (" << bestMove.coord1.x << ", " << bestMove.coord1.y
                          << ") and (" << bestMove.coord2.x << ", " << bestMove.coord2.y << ")\n";
                std::cout << "Moves undone: " << game.getBoard().counter << "\n";
                break;
            }
            case Command::Kind::Undo:
                std::cout << "Undoing last move...\n";
                game.undoMove();
                break;
            case Command::Kind::Reset:
                std::cout << "Resetting game...\n";
                game.reset();
                break;
            case Command::Kind::Quit:
                std::cout << "Quitting...\n";
                return 0;
        }
    }
}
