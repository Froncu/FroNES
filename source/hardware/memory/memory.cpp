#include "memory.hpp"

namespace nes
{
   void Memory::load_program(std::filesystem::path const& path, Word const load_address)
   {
      std::basic_ifstream<Byte, std::char_traits<Byte>> in{path.c_str(), std::ios::binary};
      in.read(&data_[load_address], static_cast<std::streamsize>(data_.size() - load_address));
   }

   void Memory::write(Word const address, Byte const data)
   {
      data_[address] = data;
   }

   auto Memory::read(Word const address) const -> Byte
   {
      return data_[address];
   }

   auto Memory::size() const -> std::size_t
   {
      return data_.size();
   }
}