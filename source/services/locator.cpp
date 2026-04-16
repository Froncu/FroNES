#include "locator.hpp"

namespace nes
{
   void Locator::remove_all()
   {
      auto& services{instance().services_};
      auto& service_indices{instance().service_indices_};

      while (not services.empty())
      {
         services.pop_back();
      }

      service_indices.clear();
   }
}