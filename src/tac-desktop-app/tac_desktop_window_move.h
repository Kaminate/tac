#pragma once


namespace Tac
{
  struct DesktopWindowRect;
  struct DesktopWindowHandle;
  void DesktopAppUpdateMove();
  void DesktopAppImplMoveControls( const DesktopWindowHandle&, const DesktopWindowRect& );
  void DesktopAppImplMoveControls( const DesktopWindowHandle& );
}
