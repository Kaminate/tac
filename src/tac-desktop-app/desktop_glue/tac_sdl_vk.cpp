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

// #include <SDL.h>
#include <SDL_vulkan.h> // SDL_Vulkan_CreateSurface

namespace Tac
{
  void SDLVkCreateSurface( VkInstance instance,
                           const void* nativeWindowHandle,
                           VkSurfaceKHR* psurface,
                           Errors& errors )
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;

    // VkInstance instance = nullptr;

    // The window must have been created with the SDL_WINDOW_VULKAN flag and instance must have been
    // created with extensions returned by SDL_Vulkan_GetInstanceExtensions() enabled.

    SDL_Window* window { nullptr };
    SDL_bool created { SDL_Vulkan_CreateSurface( window, instance, psurface ) };
    TAC_RAISE_ERROR_IF(created == SDL_FALSE, "SDL_Vulkan_CreateSurface failed", errors );
  }


  // SDL promises to deprecate the window parameter... one day
  Vector<String> GetSDLVkExtensions( // SDL_Window* window,
  Errors& errors )
  {


    SDL_Window* window { SDL_CreateWindow("", 1, 1, 1, 1, SDL_WINDOW_HIDDEN | SDL_WINDOW_VULKAN ) };
    if( !window)
    {
      const char* str { SDL_GetError()  };
      errors.Append( str );
      return {};

    }

    unsigned int count;
    const SDL_bool gotCount = SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr );
    TAC_RAISE_ERROR_IF_RETURN( gotCount != SDL_TRUE, "SDL_Vulkan_GetInstanceExtensions failed to get count", errors, {} );

    Vector< const char* > names( count );
    const SDL_bool gotNames = SDL_Vulkan_GetInstanceExtensions(window, &count, names.data() );
    TAC_RAISE_ERROR_IF_RETURN( gotNames != SDL_TRUE, "SDL_Vulkan_GetInstanceExtensions failed to get names", errors, {} );

    Vector< String > result( count );
    for( unsigned int i = 0; i < count; ++i )
      result[ i ] = names[ i ];

    SDL_DestroyWindow( window );

    return result;
  }
}
