#pragma once


namespace Tac
{
  struct DesktopWindowRect;
  struct WindowHandle;
  void DesktopAppUpdateMove();
  void DesktopAppImplMoveControls( const WindowHandle&, const DesktopWindowRect& );
  void DesktopAppImplMoveControls( const WindowHandle& );
}
