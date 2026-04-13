#include "instruction.hpp"

namespace nes
{
   Instruction::Instruction(std::coroutine_handle<promise_type> handle)
      : handle_{handle}
   {
   }

   Instruction::Instruction(Instruction&& other) noexcept
      : handle_{std::exchange(other.handle_, nullptr)}
   {
   }

   Instruction::~Instruction()
   {
      destroy_handle();
   }

   auto Instruction::operator=(Instruction&& other) noexcept -> Instruction&
   {
      destroy_handle();
      handle_ = std::exchange(other.handle_, nullptr);
      return *this;
   }

   auto Instruction::tick() const -> bool
   {
      handle_.resume();
      return handle_.done();
   }

   auto Instruction::prefetched_instruction() const -> std::optional<Instruction>&&
   {
      return std::move(handle_.promise().prefetched_instruction);
   }

   auto Instruction::destroy_handle() const -> void
   {
      if (handle_)
      {
         handle_.destroy();
      }
   }

   auto Instruction::promise_type::initial_suspend() -> std::suspend_always
   {
      return {};
   }

   auto Instruction::promise_type::final_suspend() noexcept -> std::suspend_always
   {
      return {};
   }

   auto Instruction::promise_type::unhandled_exception() -> void
   {
   }

   auto Instruction::promise_type::return_value(std::optional<Instruction> instruction) -> void
   {
      prefetched_instruction = std::move(instruction);
   }

   auto Instruction::promise_type::get_return_object() -> Instruction
   {
      return Instruction{std::coroutine_handle<promise_type>::from_promise(*this)};
   }
}