#include "src/shell/tac_desktop_vk.h"

#if __has_include( "tac_win_vk.h" )
#include "tac_win_vk.h"
#endif

namespace Tac
{
  VkSurfaceFn GetVkSurfaceFn()
  {
#if __has_include( "tac_win_vk.h" )
    return Win32VkCreateSurface;
#else
    return nullptr;
#endif
  }

  Vector<String>        GetVkExtensions()
  {
#if __has_include( "tac_win_vk.h" )
    return GetWin32VkExtensions();
#else
    return {};
#endif

  }
}
