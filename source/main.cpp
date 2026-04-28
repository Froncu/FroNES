#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "application/application.hpp"
#include "services/locator.hpp"
#include "services/logger/logger.hpp"
#include "services/visualiser/visualiser.hpp"

auto SDL_AppInit(void** const appstate, int, char** const) -> SDL_AppResult
{
   nes::Locator::provide<nes::Logger>();
   nes::Locator::provide<nes::Visualiser>();

   *appstate = new nes::Application{};
   return SDL_APP_CONTINUE;
}

auto SDL_AppIterate(void* const appstate) -> SDL_AppResult
{
   return static_cast<nes::Application*>(appstate)->update() ? SDL_APP_CONTINUE : SDL_APP_SUCCESS;
}

auto SDL_AppEvent(void* const, SDL_Event* const event) -> SDL_AppResult
{
   ImGui_ImplSDL3_ProcessEvent(event);
   if (event->type == SDL_EVENT_QUIT)
      return SDL_APP_SUCCESS;

   return SDL_APP_CONTINUE;
}

auto SDL_AppQuit(void* const appstate, SDL_AppResult const) -> void
{
   delete static_cast<nes::Application const*>(appstate);
   nes::Locator::remove_all();
}