// This file handles the resizing and moving of a window when a
// person drags at the edges.
//
// It also changes the cursor to the appropriate arrow.

#pragma once

#include "src/shell/windows/tacWindows.h"
#include "src/common/tacMemory.h"

namespace Tac
{


  struct Win32DesktopWindow;

  typedef int CursorDirType;
  enum CursorDir : CursorDirType
  {
    N = 1 << 0,
    W = 1 << 1,
    S = 1 << 2,
    E = 1 << 3,
  };


  String ToString( CursorDir cursorType );

  struct Win32Cursors
  {
    Win32Cursors();
    static Win32Cursors* Instance;
    HCURSOR cursorArrow;
    HCURSOR cursorArrowNS;
    HCURSOR cursorArrowWE;
    HCURSOR cursorArrowNE_SW;
    HCURSOR cursorArrowNW_SE;
    HCURSOR GetCursor( CursorDir cursorDir = ( CursorDir )0 );
  };

  struct Win32MouseEdgeHandler
  {
    Win32MouseEdgeHandler();
    ~Win32MouseEdgeHandler();

    void Update( Win32DesktopWindow* window );
    void ResetCursorLock();


  private:

    // care to describe what this function does?
    void SetCursorLock( CursorDir cursorDir );

    // Used to set the cursor icon
    CursorDir mCursorLock = {};
    POINT mCursorPositionOnClick = {};
    RECT mWindowRectOnClick = {};
    int edgeDistResizePx;
    int edgeDistMovePx;
    bool mEverSet = false;

    struct Handler
    {
      virtual ~Handler() = default;
      virtual void Init() {};
      virtual void Update() {};
      Win32MouseEdgeHandler* mHandler = nullptr;
      bool mIsFinished = false;
      HWND mHwnd = NULL;
    };

    struct MoveHandler : public Handler
    {
      void Update() override;
    };

    struct ResizeHandler : public Handler
    {
      void Update() override;
    };

    Handler* mHandler = nullptr;
  };

}
