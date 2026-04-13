#include "emulation_exception.hpp"

namespace nes
{
   EmulationException::EmulationException(ProgramCounter const program_counter, std::string_view const what,
      std::source_location const& location)
      : EmulatorException{what, location}
      , program_counter{program_counter}
   {
   }
}