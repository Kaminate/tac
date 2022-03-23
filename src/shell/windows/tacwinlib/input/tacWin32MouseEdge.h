// This file handles the resizing and moving of a window when a
// person drags at the edges.
//
// It also changes the cursor to the appropriate arrow.

#pragma once

#include "src/shell/windows/tacwinlib/tacWin32.h"

namespace Tac
{
  void Win32MouseEdgeInit();
  void Win32MouseEdgeUpdate();
  void Win32MouseEdgeSetMovable( const struct DesktopWindowHandle&, const struct DesktopWindowRect& );
  void Win32MouseEdgeSetResizable( const struct DesktopWindowHandle&, int );
}
