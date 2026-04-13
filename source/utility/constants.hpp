#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

namespace nes
{
#ifdef NDEBUG
   constexpr auto DEBUG{false};
#else
   constexpr auto DEBUG{true};
#endif

#ifdef __MINGW32__
   constexpr auto MINGW{true};
#else
   constexpr auto MINGW{false};
#endif
}

#endif