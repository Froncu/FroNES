#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "application.hpp"
#include "exceptions/unsupported_opcode.hpp"
#include "hardware/memory/memory.hpp"
#include "hardware/processor/processor.hpp"
#include "services/locator.hpp"
#include "services/visualiser/visualiser.hpp"

namespace nes
{
   class Application final
   {
   public:
      Application() = default;
      Application(Application const&) = delete;
      Application(Application&&) = delete;

      ~Application() = default;

      auto operator=(Application const&) -> Application& = delete;
      auto operator=(Application&&) -> Application& = delete;

      auto update() -> bool;

   private:
      auto handle_exception(UnsupportedOpcode const& exception) const -> void;

      auto try_tick() -> void;
      auto try_tick_repeatedly(std::stop_token const& stop_token) -> void;
      auto try_step() -> void;

      Visualiser& visualiser_{*Locator::get<Visualiser>()};
      Logger& logger_{*Locator::get<Logger>()};

      Memory memory_{};
      Processor processor_{memory_};
      std::jthread emulation_thread_{};
   };
}

#endif