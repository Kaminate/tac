#include "src/shell/tac_desktop_vk.h"
#include <SDL_vulkan.h> // SDL_Vulkan_CreateSurface, SDL_Window
namespace Tac
{
  void SDLVkCreateSurface( VkInstance instance,
                             const void* nativeWindowHandle,
                             VkSurfaceKHR* psurface,
                             Errors& errors );
  Vector<String> GetSDLVkExtensions( // SDL_Window* window,
                                     Errors& errors );
}
