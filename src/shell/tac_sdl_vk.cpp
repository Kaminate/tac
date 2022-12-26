#include "src/shell/vulkan/tac_renderer_vulkan.h"
//#include "src/shell/windows/tac_win32.h"
//#include "src/common/shell/tac_shell.h"
//#include "src/common/string/tac_string.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_error_handling.h"
//#include "src/common/tac_error_handling.h"
//
// #include <vulkan/vulkan_win32.h>

#include "src/shell/tac_sdl_vk.h"


namespace Tac
{
  void SDLVkCreateSurface( VkInstance instance,
                           const void* nativeWindowHandle,
                           VkSurfaceKHR* psurface,
                           Errors& errors )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;

    VkInstance instance = nullptr;

    SDL_Window* window = nullptr;
    SDL_bool created = SDL_Vulkan_CreateSurface( window, instance, psurface );
    if( created == SDL_FALSE )
    {
      errors = "SDL_Vulkan_CreateSurface failed!";
    }

    // !!!
    // SDL_CreateWindow(... SDL_WINDOW_VULKAN);

    static int asdf;
    ++asdf;
  }

  Vector<String> GetSDLVkExtensions()
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
    static int asdf;
    ++asdf;
    return { };
  }
}
