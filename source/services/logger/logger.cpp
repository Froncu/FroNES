#include "logger.hpp"
#include "utility/hash.hpp"

namespace std
{
   auto hash<source_location>::operator()(source_location const& location) const noexcept -> size_t
   {
      size_t const hash_1{ nes::hash(location.file_name()) };
      size_t const hash_2{ nes::hash(location.line()) };
      size_t const hash_3{ nes::hash(location.column()) };
      size_t const hash_4{ nes::hash(location.function_name()) };

      size_t seed{};
      auto const generate{
         [&seed](size_t const hash) -> void
         {
            seed ^= hash + 0x9e'37'79'b9 + (seed << 6) + (seed >> 2);
         }
      };

      generate(hash_1);
      generate(hash_2);
      generate(hash_3);
      generate(hash_4);

      return seed;
   }

   auto equal_to<source_location>::operator()(source_location const& location_a, source_location const& location_b) const -> bool
   {
      if (location_a.line() not_eq location_b.line() or location_a.column() not_eq location_b.column())
         return false;

      if (location_a.file_name() == location_b.file_name() and location_a.function_name() == location_b.function_name())
         return true;

      return not std::strcmp(location_a.file_name(), location_b.file_name())
         and not std::strcmp(location_a.function_name(), location_b.function_name());
   }
}

namespace nes
{
   Logger::Logger(Locator::ConstructionKey)
   {
   }

   Logger::~Logger()
   {
      thread_.request_stop();
      condition_.notify_one();
   }

   auto Logger::log(Payload const& payload) -> void
   {
      std::ostream* output_stream;
      switch (payload.type)
      {
         case Type::INFO:
            output_stream = &std::clog;
            break;

         case Type::WARNING:
            [[fallthrough]];

         case Type::ERROR:
            output_stream = &std::cerr;
            break;

         default:
            output_stream = &std::cout;
      }

      std::string_view escape_sequence;
      std::string_view type;
      switch (payload.type)
      {
         case Type::INFO:
            escape_sequence = "1;36";
            type = "INFO";
            break;

         case Type::WARNING:
            escape_sequence = "1;33";
            type = "WARNING";
            break;

         case Type::ERROR:
            escape_sequence = "1;31";
            type = "ERROR";
            break;
      }

      std::println(*output_stream,
         "[\x1b[{}m{}\x1b[0m] [\x1b[1;97m{}({})\x1b[0m]\n{}",
         escape_sequence,
         type,
         payload.location.file_name(),
         payload.location.line(),
         payload.message);
   }

   auto Logger::log_once(Payload const& payload) -> void
   {
      if (not location_entries_.insert(payload.location).second)
         return;

      return log(payload);
   }
}