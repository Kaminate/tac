#pragma once
#include "src/common/containers/tac_vector.h"
#include "src/common/string/tac_string.h"
#include "src/common/tac_core.h"

#include <vulkan/vulkan.h>

namespace Tac
{

  using VkSurfaceFn = void ( * )( VkInstance,
                                 const void* nativeWindowHandle,
                                 //const DesktopWindowHandle&,
                                 VkSurfaceKHR*,
                                 Errors& );

  // This GetVkSurfaceFn and GetVkExtensions are supposed to be platform-agnostic,
  // so there will exist a Win32 and SDL version of them.
  auto GetVkSurfaceFn()->VkSurfaceFn;
  auto GetVkExtensions( Errors& ) -> Vector<String>;


}

