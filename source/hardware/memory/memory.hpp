#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "hardware/types.hpp"
#include "pch.hpp"

namespace nes
{
   class Memory final
   {
      public:
         Memory() = default;
         Memory(Memory const&) = delete;
         Memory(Memory&&) = delete;

         ~Memory() = default;

         auto operator=(Memory const&) -> Memory& = delete;
         auto operator=(Memory&&) -> Memory& = delete;

         auto load_program(std::filesystem::path const& path, Word load_address = 0x00'00) -> void;

         auto write(Word address, Byte data) -> void;
         [[nodiscard]] auto read(Word address) const -> Byte;

         [[nodiscard]] auto size() const -> std::size_t;

      private:
         std::array<Byte, std::numeric_limits<ProgramCounter>::max() + 1> data_{};
   };
}

#endif