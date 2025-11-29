// This file handles the resizing and moving of a window when a
// person drags at the edges.
//
// It also changes the cursor to the appropriate arrow.

#pragma once

#include "tac-win32/tac_win32.h"

namespace Tac { struct WindowHandle; struct DesktopWindowRect; }
namespace Tac
{
  void Win32MouseEdgeInit();
  void Win32MouseEdgeUpdate();
  void Win32MouseEdgeSetMovable( const WindowHandle&, const DesktopWindowRect& );
  void Win32MouseEdgeSetResizable( const WindowHandle&, int );
  auto Win32MouseEdgeGetCursorHovered() -> WindowHandle;
}
