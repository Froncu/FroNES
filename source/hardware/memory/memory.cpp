#include "memory.hpp"

namespace nes
{
   void Memory::load_program(std::filesystem::path const& path, Word const load_address)
   {
      std::ifstream in{path.c_str(), std::ios::binary};
      std::array<char, std::size(decltype(data_){})> buffer{};
      in.read(buffer.data(), buffer.size());
      std::memcpy(&data_[load_address], buffer.data(), buffer.size() - load_address);
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