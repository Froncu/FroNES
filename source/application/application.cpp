#include "application.hpp"
#include "services/visualiser/visualiser.hpp"

namespace nes
{
   auto Application::update() -> bool
   {
      if (not visualiser_.update(memory_, processor_))
         return false;

      if (visualiser_.tick_repeatedly())
      {
         if (not emulation_thread_.joinable())
            emulation_thread_ = std::jthread{ std::bind_front(&Application::try_tick_repeatedly, this) };
      }
      else if (emulation_thread_.joinable())
      {
         emulation_thread_.request_stop();
         emulation_thread_.join();
      }
      else if (visualiser_.tick_once())
         try_tick();
      else if (visualiser_.step())
         try_step();
      else if (visualiser_.reset())
         processor_.reset();

      if (visualiser_.load_program_requested())
         memory_.load_program(visualiser_.program_path(), visualiser_.program_load_address());

      return true;
   }

   auto Application::handle_exception(UnsupportedOpcode const& exception) const -> void
   {
      logger_.error(exception.what(), false, exception.location);
   }

   auto Application::try_tick() -> void try
   {
      processor_.tick();
   }
   catch (UnsupportedOpcode const& exception)
   {
      handle_exception(exception);
   }

   auto Application::try_tick_repeatedly(std::stop_token const& stop_token) -> void
   {
      while (not stop_token.stop_requested())
         try_tick();
   }

   auto Application::try_step() -> void try
   {
      while (not processor_.tick());
   }
   catch (UnsupportedOpcode const& exception)
   {
      handle_exception(exception);
   }
}