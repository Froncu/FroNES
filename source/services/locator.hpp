// Inspired by Matias Devred's locator from his "gooey" engine
// (https://git.allpurposem.at/mat/gooey/src/branch/main/src/lib/services/locator.cppm)

#ifndef LOCATOR_HPP
#define LOCATOR_HPP

#include "pch.hpp"
#include "utility/type_index.hpp"
#include "utility/unique_pointer.hpp"
#include "utility/void_deleter.hpp"

namespace nes
{
   // TODO: not a fan of the `instance` approach, but I don't like having the user to do `Locator::instance()` either.
   class Locator final
   {
   public:
      template<typename Service, std::derived_from<Service> Provider = Service, typename... Arguments>
      requires std::constructible_from<Provider, Arguments...>
      static auto provide(Arguments&&... arguments) -> Service&
      {
         auto& services{instance().services_};
         auto& service_indices{instance().service_indices_};

         UniquePointer<void> new_provider{new Provider{std::forward<Arguments>(arguments)...}, void_deleter<Provider>};

         auto&& [service_index, did_insert]{service_indices.emplace(type_index<Service>(), services.size())};
         if (did_insert)
         {
            return *static_cast<Service*>(services.emplace_back(std::move(new_provider)).get());
         }

         UniquePointer<void>& current_provider{services[service_index->second]};

         if constexpr (std::movable<Service>)
         {
            *static_cast<Service*>(new_provider.get()) = std::move(*static_cast<Service*>(current_provider.get()));
         }

         current_provider = std::move(new_provider);
         return *static_cast<Service*>(current_provider.get());
      }

      static void remove_providers()
      {
         auto& services{instance().services_};
         auto& service_indices{instance().service_indices_};

         while (not services.empty())
         {
            services.pop_back();
         }

         service_indices.clear();
      }

      template<typename Service>
      [[nodiscard]] static auto get() -> Service*
      {
         auto& services{instance().services_};
         auto& service_indices{instance().service_indices_};

         auto const service_index{service_indices.find(type_index<Service>())};
         if (service_index == service_indices.end())
         {
            return nullptr;
         }

         return static_cast<Service* const>(services[service_index->second].get());
      }

      Locator(Locator const&) = delete;
      Locator(Locator&&) = delete;

      auto operator=(Locator const&) -> Locator& = delete;
      auto operator=(Locator&&) -> Locator& = delete;

   private:
      [[nodiscard]] static auto instance() -> Locator&
      {
         static Locator instance{};
         return instance;
      }

      Locator() = default;

      ~Locator() = default;

      std::unordered_map<std::type_index, std::size_t> service_indices_{};
      std::vector<UniquePointer<void>> services_{};
   };
}

#endif