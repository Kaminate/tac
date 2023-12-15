// This file handles the resizing and moving of a window when a
// person drags at the edges.
//
// It also changes the cursor to the appropriate arrow.

#pragma once

#include "src/common/tac_core.h"
#include "src/shell/windows/tac_win32.h"

namespace Tac
{
  void Win32MouseEdgeInit();
  void Win32MouseEdgeUpdate();
  void Win32MouseEdgeSetMovable( const DesktopWindowHandle&, const DesktopWindowRect& );
  void Win32MouseEdgeSetResizable( const DesktopWindowHandle&, int );
  DesktopWindowHandle Win32MouseEdgeGetCursorHovered();
}
