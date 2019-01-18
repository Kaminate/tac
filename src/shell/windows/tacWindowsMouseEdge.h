// This file handles the resizing and moving of a window when a
// person drags at the edges.
//
// It also changes the cursor to the appropriate arrow.

#pragma once

#include "tacWindows.h"
//#include "common/tacUtility.h"
//#include "common/tacEvent.h"
#include "common/tacMemory.h"
#include <map>

struct TacWin32DesktopWindow;
struct TacWindowsApplication2;
struct TacKeyboardInput;

typedef int TacCursorDirType;
enum TacCursorDir : TacCursorDirType
{
  N = 1 << 0,
  W = 1 << 1,
  S = 1 << 2,
  E = 1 << 3,
};


TacString ToString( TacCursorDir cursorType );

struct TacWin32MouseEdgeCommonData
{
  TacWin32MouseEdgeCommonData();
  HCURSOR GetCursor( TacCursorDir cursorDir );
  HCURSOR cursorArrow;
  HCURSOR cursorArrowNS;
  HCURSOR cursorArrowWE;
  HCURSOR cursorArrowNE_SW;
  HCURSOR cursorArrowNW_SE;
  int edgeDistResizePx;
  int edgeDistMovePx;
  std::map< TacCursorDir, HCURSOR > mCursors;
};


struct TacWin32MouseEdgeHandler
{
  TacKeyboardInput* mKeyboardInput = nullptr;
  TacWin32MouseEdgeCommonData* mMouseEdgeCommonData = nullptr;

  //void Update( TacVector< TacWin32DesktopWindow*>& windows );
  void Update( HWND parentHwnd );
  void ResetCursorLock();

private:

  void SetCursorLock( TacCursorDir cursorDir );

  // Used to set the cursor icon
  TacCursorDir mCursorLock = {};
  POINT mCursorPositionOnClick = {};
  RECT mWindowRectOnClick = {};

  struct Handler
  {
    virtual ~Handler() = default;
    virtual void Init() {};
    virtual void Update() {};
    TacWin32MouseEdgeHandler* mHandler = nullptr;
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

  TacOwned< Handler > mHandler;

};

