#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Command.hpp"

class CommandProcessor {
   public:
    static Command waitForCommand();

   private:
    static std::optional<Command> processCommand(const std::string& command);
};
