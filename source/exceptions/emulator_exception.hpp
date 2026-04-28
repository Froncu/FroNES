#ifndef EMULATOR_EXCEPTION_HPP
#define EMULATOR_EXCEPTION_HPP

#include "pch.hpp"

namespace nes
{
   class EmulatorException : public std::runtime_error
   {
      public:
         EmulatorException(std::string_view what, std::source_location const& location = std::source_location::current());
         EmulatorException(EmulatorException const&) = default;
         EmulatorException(EmulatorException&&) = delete;

         ~EmulatorException() override = default;

         auto operator=(EmulatorException const&) -> EmulatorException& = delete;
         auto operator=(EmulatorException&&) -> EmulatorException& = delete;

         std::source_location const location;
   };
}

#endif