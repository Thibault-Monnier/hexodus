#include "CommandProcessor.hpp"

#include <iostream>
#include <sstream>
#include <string>

Command CommandProcessor::waitForCommand() {
    std::string line;
    while (true) {
        std::getline(std::cin, line);

        if (const std::optional<Command> command = processCommand(line)) {
            return command.value();
        }
    }
}

std::optional<Command> CommandProcessor::processCommand(const std::string& command) {
    std::istringstream iss(command);
    iss >> std::ws;  // Skip leading whitespace

    std::string commandKind;
    iss >> commandKind >> std::ws;

    if (commandKind.empty()) {
        // No command, ignore
        return std::nullopt;
    }

    if (commandKind == "ping") {
        return Command(Command::Kind::Ping);
    }
    if (commandKind == "test") {
        return Command(Command::Kind::Test);
    }
    if (commandKind == "quit") {
        return Command(Command::Kind::Quit);
    }

    std::cerr << "Unknown command is ignored: " << command << '\n';
    return std::nullopt;
}
