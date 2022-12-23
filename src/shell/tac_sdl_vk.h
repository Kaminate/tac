#include "src/shell/vulkan/tac_renderer_vulkan.h"
#include "src/shell/windows/tac_win32.h"
//#include "src/common/shell/tac_shell.h"
//#include "src/common/string/tac_string.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_error_handling.h"
//#include "src/common/tac_error_handling.h"
//
#include <vulkan/vulkan_win32.h>


namespace Tac
{
  void TacVulkanWin32CreateSurface( VkInstance instance,
                                    DesktopWindowHandle desktopWindowHandle,
                                    VkSurfaceKHR* psurface,
                                    Errors& errors );

//
//static int vulkanwin32stuff = []() {
//  auto vg = TacVulkanGlobals::Instance();
//  vg->mRequiredExtensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
//  vg->mCreateSurface = TacVulkanWin32CreateSurface;
//  return 0;
//}( );
//
//
}
