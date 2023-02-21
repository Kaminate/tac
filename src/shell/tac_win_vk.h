#include "src/shell/tac_desktop_vk.h"
#include "src/common/tac_common.h"

namespace Tac
{
  void Win32VkCreateSurface( VkInstance instance,
                             const void* nativeWindowHandle,
                             //HWND hwnd,
                             //const DesktopWindowHandle& desktopWindowHandle,
                             VkSurfaceKHR* psurface,
                             Errors& errors );
  Vector<String> GetWin32VkExtensions( Errors& );
}
