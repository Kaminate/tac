#pragma once


namespace Tac
{
  struct DesktopWindowHandle;

  void DesktopAppUpdateResize();
  void DesktopAppImplResizeControls( const DesktopWindowHandle&, int edgePx );
}
