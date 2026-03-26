#pragma once

class Command {
   public:
    enum class Kind : uint8_t { Ping, Test, Quit };

   private:
    Kind kind_;

   public:
    explicit Command(const Kind kind) : kind_(kind) {}

    [[nodiscard]] Kind getKind() const { return kind_; }
};
