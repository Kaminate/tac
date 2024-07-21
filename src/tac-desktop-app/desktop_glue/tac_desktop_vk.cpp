#include "src/shell/tac_desktop_vk.h"

// hmm maybe the best way to do this is with cmake target_compile_definitions


#if /*defined( _WIN32 )  && */ defined(TAC_USE_DESKTOP_WIN32) && __has_include( "tac_win_vk.h" )
#define TAC_INCLUDED_WIN_VK true
#include "tac_win_vk.h"
#else
#define TAC_INCLUDED_WIN_VK false
#endif

#if defined(TAC_USE_DESKTOP_SDL ) && __has_include( "tac_sdl_vk.h" )
#define TAC_INCLUDED_SDL_VK true
#include "tac_sdl_vk.h"
#else
#define TAC_INCLUDED_SDL_VK false
#endif




namespace Tac
{
  VkSurfaceFn GetVkSurfaceFn()
  {
    VkSurfaceFn fn {};

#if TAC_INCLUDED_WIN_VK
    fn = Win32VkCreateSurface;
#endif

#if TAC_INCLUDED_SDL_VK
    fn = SDLVkCreateSurface;
#endif

    return fn;
  }

  Vector<String>        GetVkExtensions( Errors& errors)
  {
    Vector<String> ext;
#if TAC_INCLUDED_WIN_VK
    ext = GetWin32VkExtensions( errors);
#endif

#if TAC_INCLUDED_SDL_VK
    ext = GetSDLVkExtensions( errors );
#endif
    return ext;

  }
}
