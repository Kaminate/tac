#pragma once


namespace Tac
{
  struct WindowHandle;

  void DesktopAppUpdateResize();
  void DesktopAppImplResizeControls( const WindowHandle&, int edgePx );
}
