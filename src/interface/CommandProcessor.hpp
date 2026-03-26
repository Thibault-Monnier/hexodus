#pragma once

#include <memory>
#include <string>

#include "Command.hpp"

class CommandProcessor {
   public:
    static std::unique_ptr<Command> waitForCommand();

   private:
    static std::unique_ptr<Command> processCommand(const std::string& command);
};
