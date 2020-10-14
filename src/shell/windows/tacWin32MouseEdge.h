// This file handles the resizing and moving of a window when a
// person drags at the edges.
//
// It also changes the cursor to the appropriate arrow.

#pragma once

#include "src/shell/windows/tacWin32.h"

namespace Tac
{
  typedef int CursorDir;
  const CursorDir CursorDirN = 0b0001;
  const CursorDir CursorDirW = 0b0010;
  const CursorDir CursorDirS = 0b0100;
  const CursorDir CursorDirE = 0b1000;

  String CursorDirToString( CursorDir cursorType );
  void SetCursor( CursorDir );

  struct Win32MouseEdgeHandler
  {
    static Win32MouseEdgeHandler Instance;
    Win32MouseEdgeHandler();

    void Update( HWND );
    void ResetCursorLock();


  private:

    enum HandlerType
    {
      None,
      Move,
      Resize,
    };

    // care to describe what this function does?
    void SetCursorLock( CursorDir );

    void UpdateIdle( HWND );
    void UpdateResize();
    void UpdateMove();

    // Used to set the cursor icon
    CursorDir mCursorLock = {};
    POINT mCursorPositionOnClick = {};
    RECT mWindowRectOnClick = {};
    int edgeDistResizePx;
    int edgeDistMovePx;
    bool mEverSet = false;
    HandlerType mHandlerType = HandlerType::None;
    bool mIsFinished = false;
    HWND mHwnd = NULL;

    bool mMouseDownCurr = false;
    bool mMouseDownPrev = false;
  };

}
