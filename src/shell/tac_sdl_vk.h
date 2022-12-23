#include "src/shell/tac_desktop_vk.h"
namespace Tac
{
  void SDLVkCreateSurface( VkInstance instance,
                             const void* nativeWindowHandle,
                             VkSurfaceKHR* psurface,
                             Errors& errors );
  Vector<String> GetSDLVkExtensions();
}
