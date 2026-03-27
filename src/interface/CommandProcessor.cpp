#include "CommandProcessor.hpp"

#include <iostream>
#include <sstream>
#include <string>

#include "../core/Chunk.hpp"

std::unique_ptr<Command> CommandProcessor::waitForCommand() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (std::unique_ptr<Command> command = processCommand(line)) {
            return command;
        }
    }

    // EOF reached, return a quit command
    return std::make_unique<Command>(Command::Kind::Quit);
}

std::unique_ptr<Command> CommandProcessor::processCommand(const std::string& command) {
    std::istringstream iss(command);
    iss >> std::ws;  // Skip leading whitespace

    std::string commandKind;
    iss >> commandKind >> std::ws;
    std::ranges::transform(commandKind, commandKind.begin(), ::tolower);

    if (commandKind.empty()) {
        // No command, ignore
        return nullptr;
    }

    if (commandKind == "print") {
        return std::make_unique<Command>(Command::Kind::Print);
    }

    if (commandKind == "move") {
        int16_t x1, y1, x2, y2;
        if (iss >> x1 >> y1 >> x2 >> y2) {
            return std::make_unique<MoveCommand>(Coordinate(x1, y1), Coordinate(x2, y2));
        }

        std::cerr << "Invalid move command format is ignored: " << command << '\n';
        return nullptr;
    }

    if (commandKind == "moverequest") {
        return std::make_unique<Command>(Command::Kind::MoveRequest);
    }

    if (commandKind == "analyse") {
        return std::make_unique<Command>(Command::Kind::Analyse);
    }

    if (commandKind == "quit") {
        return std::make_unique<Command>(Command::Kind::Quit);
    }

    std::cerr << "Unknown command is ignored: " << command << '\n';
    return nullptr;
}
