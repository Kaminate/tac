#include "src/shell/tac_win_vk.h" // self-inc

#include "src/shell/vulkan/tac_renderer_vulkan.h" // TAC_VK_CALL
#include "src/shell/windows/tac_win32.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/error/tac_error_handling.h"

#include <vulkan/vulkan_win32.h>

namespace Tac
{
  void Win32VkCreateSurface( VkInstance instance,
                             const void* nativeWindowHandle,
                             VkSurfaceKHR* psurface,
                             Errors& errors )
  {
    const HINSTANCE hInstance = Win32GetStartupInstance();
    TAC_ASSERT( hInstance );

    const HWND hWnd = ( HWND )nativeWindowHandle;
    TAC_ASSERT( hWnd );

    const VkWin32SurfaceCreateInfoKHR surface_create_info =
    {
      .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
      .hinstance = hInstance,
      .hwnd = hWnd,
    };
    TAC_VK_CALL( errors, vkCreateWin32SurfaceKHR, instance, &surface_create_info, nullptr, psurface );
  }

  Vector<String> GetWin32VkExtensions( Errors& )
  {
    Vector<String> exts;
    exts.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
    return exts;
  }
}
