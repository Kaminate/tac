//#include "src/shell/vulkan/tacRendererVulkan.h"
//#include "src/shell/windows/tac_win32.h"
//#include "src/common/shell/tac_shell.h"
//#include "src/common/string/tac_string.h"
//#include "src/common/tac_desktop_window.h"
//#include "src/common/tac_error_handling.h"
//
//#include <vulkan/vulkan_win32.h>
//
//
//void TacVulkanWin32CreateSurface( TacShell* shell,
//                                  VkInstance instance,
//                                  TacDesktopWindow* desktopWindow,
//                                  VkSurfaceKHR *psurface,
//                                  TacErrors& errors )
//{
//  auto hInstance = ( HINSTANCE )desktopWindow->mOperatingSystemApplicationHandle;
//  TacAssert( hInstance );
//
//  auto hWnd = ( HWND )desktopWindow->mOperatingSystemHandle;
//  TacAssert( hWnd );
//
//  VkWin32SurfaceCreateInfoKHR surface_create_info = {};
//  surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
//  surface_create_info.pNext;
//  surface_create_info.flags;
//  surface_create_info.hinstance = hInstance;
//  surface_create_info.hwnd = hWnd;
//
//  TAC_VULKAN_CALL( errors, vkCreateWin32SurfaceKHR, instance, &surface_create_info, nullptr, psurface );
//}
//
//static int vulkanwin32stuff = []() {
//  auto vg = TacVulkanGlobals::Instance();
//  vg->mRequiredExtensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
//  vg->mCreateSurface = TacVulkanWin32CreateSurface;
//  return 0;
//}( );
//
//
