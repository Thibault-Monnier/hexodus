#include <iostream>

#include "core/Board.hpp"
#include "core/Game.hpp"
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
                const Move bestMove = game.playBestMove(2);
                std::cout << "Best move: (" << bestMove.getCoord1().x << ", "
                          << bestMove.getCoord1().y << ") and (" << bestMove.getCoord2().x << ", "
                          << bestMove.getCoord2().y << ")\n";
                break;
            }
            case Command::Kind::Analyse: {
                const Move bestMove = game.findBestMove(2);
                std::cout << "Best move: (" << bestMove.getCoord1().x << ", "
                          << bestMove.getCoord1().y << ") and (" << bestMove.getCoord2().x << ", "
                          << bestMove.getCoord2().y << ")\n";
                break;
            }
            case Command::Kind::Quit:
                std::cout << "Quitting...\n";
                return 0;
        }
    }
}
