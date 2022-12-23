#pragma once
#include "src/common/containers/tac_vector.h"
#include "src/common/string/tac_string.h"

#include <vulkan/vulkan.h>

namespace Tac
{
  struct Errors;
  struct DesktopWindowHandle;

  typedef void ( *VkSurfaceFn )( VkInstance,
                                 const void* nativeWindowHandle,
                                 //const DesktopWindowHandle&,
                                 VkSurfaceKHR*,
                                 Errors& );

  VkSurfaceFn    GetVkSurfaceFn();
  Vector<String> GetVkExtensions();
}

