#ifndef VISUALISER_HPP
#define VISUALISER_HPP

#include "hardware/processor/processor.hpp"
#include "pch.hpp"
#include "utility/runtime_assert.hpp"

namespace nes
{
   class Visualiser final
   {
      class SDL_Context final
      {
      public:
         SDL_Context();
         SDL_Context(SDL_Context const&) = delete;
         SDL_Context(SDL_Context&&) = delete;

         ~SDL_Context();

         auto operator=(SDL_Context const&) -> SDL_Context& = delete;
         auto operator=(SDL_Context&&) -> SDL_Context& = delete;

      private:
         SDL_InitFlags const initialisation_flags_{SDL_INIT_VIDEO};
      };

      class ImGuiBackend final
      {
      public:
         ImGuiBackend(SDL_Window& window, SDL_Renderer& renderer);
         ImGuiBackend(ImGuiBackend const&) = delete;
         ImGuiBackend(ImGuiBackend&&) = delete;

         ~ImGuiBackend();

         auto operator=(ImGuiBackend const&) -> ImGuiBackend& = delete;
         auto operator=(ImGuiBackend&&) -> ImGuiBackend& = delete;

         UniquePointer<ImGuiContext> const context{ImGui::CreateContext(), ImGui::DestroyContext};
      };

   public:
      Visualiser() = default;
      Visualiser(Visualiser const&) = delete;
      Visualiser(Visualiser&&) = delete;

      ~Visualiser() = default;

      auto operator=(Visualiser const&) -> Visualiser& = delete;
      auto operator=(Visualiser&&) -> Visualiser& = delete;

      [[nodiscard]] auto update(Memory const& memory, Processor& processor) -> bool;

      [[nodiscard]] auto tick_repeatedly() const -> bool;
      [[nodiscard]] auto tick_once() const -> bool;
      [[nodiscard]] auto step() const -> bool;
      [[nodiscard]] auto reset() const -> bool;

      [[nodiscard]] auto program_path() const -> std::filesystem::path const&;
      [[nodiscard]] auto program_load_address() const -> Word;
      [[nodiscard]] auto load_program_requested() const -> bool;

   private:
      SDL_Context const context_{};

      UniquePointer<SDL_Window> const window_{
         [] -> SDL_Window*
         {
            SDL_Window* const window{SDL_CreateWindow("Emulator", 1'280, 720, SDL_WINDOW_RESIZABLE)};
            runtime_assert(window, std::format("failed to create window ({})", SDL_GetError()));

            return window;
         }(),
         SDL_DestroyWindow
      };

      UniquePointer<SDL_Renderer> const renderer_{
         [this] -> SDL_Renderer*
         {
            SDL_Renderer* const renderer{SDL_CreateRenderer(window_.get(), nullptr)};
            runtime_assert(renderer, std::format("failed to create renderer ({})", SDL_GetError()));

            return renderer;
         }(),
         SDL_DestroyRenderer
      };

      ImGuiBackend const imgui_backend_{*window_, *renderer_};

      Word jump_address_{};
      int bytes_per_row_{16};
      int visible_rows_{16};
      bool jump_requested_{};
      std::filesystem::path program_path_{};
      Word program_load_address_{};
      bool load_program_requested_{};

      bool tick_repeatedly_{};
      bool tick_once_{};
      bool step_{};
      bool reset_{};
   };
}

#endif