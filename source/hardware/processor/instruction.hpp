#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include "pch.hpp"

namespace nes
{
   class Instruction final
   {
   public:
      struct promise_type;

      explicit Instruction(std::coroutine_handle<promise_type> handle);
      Instruction(Instruction const&) = delete;
      Instruction(Instruction&& other) noexcept;

      ~Instruction();

      auto operator=(Instruction const&) -> Instruction& = delete;
      auto operator=(Instruction&& other) noexcept -> Instruction&;

      [[nodiscard]] auto tick() const -> bool;
      [[nodiscard]] auto prefetched_instruction() const -> std::optional<Instruction>&&;

   private:
      auto destroy_handle() const -> void;

      std::coroutine_handle<promise_type> handle_;
   };

   struct Instruction::promise_type
   {
      static auto initial_suspend() -> std::suspend_always;
      static auto final_suspend() noexcept -> std::suspend_always;
      static auto unhandled_exception() -> void;
      auto return_value(std::optional<Instruction> instruction) -> void;

      auto get_return_object() -> Instruction;

      std::optional<Instruction> prefetched_instruction{};
   };
}

#endif