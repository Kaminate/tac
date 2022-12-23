//#include "src/shell/vulkan/tac_renderer_vulkan.h"
//#include "src/shell/windows/tac_win32.h"
//#include "src/common/shell/tac_shell.h"
//#include "src/common/string/tac_string.h"
//#include "src/common/tac_desktop_window.h"
//#include "src/common/tac_error_handling.h"
//#include "src/common/tac_error_handling.h"
//
//#include <vulkan/vulkan_win32.h>
//#include <vulkan/vulkan.h>
#include "src/shell/tac_desktop_vk.h"

namespace Tac
{
  struct DesktopWindowHandle;
  struct Errors;
  void Win32VkCreateSurface( VkInstance instance,
                             const void* nativeWindowHandle,
                             //HWND hwnd,
                             //const DesktopWindowHandle& desktopWindowHandle,
                             VkSurfaceKHR* psurface,
                             Errors& errors );
  Vector<String> GetWin32VkExtensions();
}
