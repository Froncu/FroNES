#ifndef UNSUPPORTED_OPCODE_HPP
#define UNSUPPORTED_OPCODE_HPP

#include "emulation_exception.hpp"

namespace nes
{
   class UnsupportedOpcode final : public EmulationException
   {
      public:
         UnsupportedOpcode(ProgramCounter program_counter, Byte opcode, std::source_location const& location = std::source_location::current());
         UnsupportedOpcode(UnsupportedOpcode const&) = default;
         UnsupportedOpcode(UnsupportedOpcode&&) = delete;

         ~UnsupportedOpcode() override = default;

         auto operator=(UnsupportedOpcode const&) -> UnsupportedOpcode& = delete;
         auto operator=(UnsupportedOpcode&&) -> UnsupportedOpcode& = delete;

         Byte const opcode;
   };
}

#endif