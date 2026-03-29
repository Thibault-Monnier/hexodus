#pragma once

#include "core/board/Chunk.hpp"

class Command {
   public:
    enum class Kind : uint8_t { Print, Move, MoveRequest, Analyse, Undo, Reset, Quit };

   private:
    Kind kind_;

   public:
    explicit Command(const Kind kind) : kind_(kind) {}
    virtual ~Command() = default;

    [[nodiscard]] Kind getKind() const { return kind_; }
};

class MoveCommand : public Command {
    Coordinate coord1_, coord2_;

   public:
    MoveCommand(const Coordinate coord1, const Coordinate coord2)
        : Command(Command::Kind::Move), coord1_(coord1), coord2_(coord2) {}

    [[nodiscard]] Coordinate getCoord1() const { return coord1_; }
    [[nodiscard]] Coordinate getCoord2() const { return coord2_; }
};
