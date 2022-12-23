#include "src/shell/vulkan/tac_renderer_vulkan.h" // TAC_VK_CALL
#include "src/shell/tac_win_vk.h"
#include "src/shell/windows/tac_win32.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_error_handling.h"
#include <vulkan/vulkan_win32.h>

namespace Tac
{
  void Win32VkCreateSurface( VkInstance instance,
                             //HWND hWnd,
                             const void* nativeWindowHandle,
                             //const DesktopWindowHandle& desktopWindowHandle,
                             VkSurfaceKHR* psurface,
                             Errors& errors )
  {
    //const DesktopWindowState* state = GetDesktopWindowState( desktopWindowHandle );
    const HINSTANCE hInstance = Win32GetStartupInstance();
    //const HWND hWnd = ( HWND )state->mNativeWindowHandle;
    const HWND hWnd = ( HWND )nativeWindowHandle;
    TAC_ASSERT( hInstance );
    TAC_ASSERT( hWnd );
    const VkWin32SurfaceCreateInfoKHR surface_create_info =
    {
      .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
      .hinstance = hInstance,
      .hwnd = hWnd,
    };
    TAC_VK_CALL( errors, vkCreateWin32SurfaceKHR, instance, &surface_create_info, nullptr, psurface );
  }

  Vector<String> GetWin32VkExtensions()
  {
    return { VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
  }
}
