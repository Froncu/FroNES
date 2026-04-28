#include "emulator_exception.hpp"

namespace nes
{
   EmulatorException::EmulatorException(std::string_view const what, std::source_location const& location)
      : std::runtime_error{ what.data() }
      , location{ location }
   {
   }
}