#include "src/shell/tac_desktop_vk.h"
#include "src/common/tac_common.h"

namespace Tac
{
  void Win32VkCreateSurface( VkInstance ,
                             const void* nativeWindowHandle,
                             //HWND hwnd,
                             //const DesktopWindowHandle& desktopWindowHandle,
                             VkSurfaceKHR* ,
                             Errors& );
  Vector<String> GetWin32VkExtensions( Errors& );
}
