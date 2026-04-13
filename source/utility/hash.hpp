#ifndef HASH_HPP
#define HASH_HPP

#include "pch.hpp"

namespace nes
{
   template<typename Value>
   auto hash(Value const& value) -> std::size_t
   {
      return std::hash<Value>{}(value);
   }
}

#endif