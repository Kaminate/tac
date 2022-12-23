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
