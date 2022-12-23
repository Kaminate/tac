#include "src/shell/tac_desktop_vk.h"

// hmm maybe the best way to do this is with cmake target_compile_definitions

#if defined( _WIN32 ) && __has_include( "tac_win_vk.h" )
#include "tac_win_vk.h"
#endif

#if __has_include( "tac_sdl_vk.h" )
#include "tac_sdl_vk.h"
#endif

namespace Tac
{
  VkSurfaceFn GetVkSurfaceFn()
  {
#if defined( _WIN32 ) &&__has_include( "tac_win_vk.h" )
    return Win32VkCreateSurface;
#elif __has_include( "tac_sdl_vk.h" )
  return SDLVkCreateSurface;
#else
    return nullptr;
#endif
  }

  Vector<String>        GetVkExtensions()
  {
#if defined( _WIN32 ) &&__has_include( "tac_win_vk.h" )
    return GetWin32VkExtensions();
#elif __has_include( "tac_sdl_vk.h" )
  return GetSDLVkExtensions();
#else
    return {};
#endif

  }
}
