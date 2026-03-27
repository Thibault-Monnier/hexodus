#pragma once

#include <memory>
#include <string>

#include "Command.hpp"

class CommandProcessor {
   public:
    /// Waits for a valid command to be entered via standard input, parses it then returns a
    /// unique_ptr to that command. The returned pointer is guaranteed to be non-null.
    static std::unique_ptr<Command> waitForCommand();

   private:
    /// Parses the input string and returns a unique_ptr to the corresponding Command object if it
    /// is valid, or nullptr otherwise.
    static std::unique_ptr<Command> processCommand(const std::string& command);
};
