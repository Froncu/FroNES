#ifndef EMULATION_EXCEPTION_HPP
#define EMULATION_EXCEPTION_HPP

#include "emulator_exception.hpp"
#include "hardware/types.hpp"

namespace nes
{
   class EmulationException: public EmulatorException
   {
   public:
      EmulationException(ProgramCounter program_counter, std::string_view what,
         std::source_location const& location = std::source_location::current());
      EmulationException(EmulationException const&) = default;
      EmulationException(EmulationException&&) = delete;

      ~EmulationException() override = default;

      auto operator=(EmulationException const&) -> EmulationException& = delete;
      auto operator=(EmulationException&&) -> EmulationException& = delete;

      ProgramCounter const program_counter;
   };
}

#endif