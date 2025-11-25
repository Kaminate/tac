#pragma once

#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/hid/tac_sys_keyboard_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/math/tac_vector2i.h"

namespace Tac
{
  struct MouseEdgeApi
  {
    struct State
    {
    };

    enum class Side
    {
      kNone = 0,
      kRight = 1 >> 0,
      kLeft = 1 >> 1,
      kBot = 1 >> 2,
      kTop = 1 >> 3,
    };

    struct Input
    {
      WindowHandle mWindowHandle;
    };

    struct Output
    {
      Side mSide{ Side::kNone };
    };


    void UpdateSim( Input input )
    {
      const WindowHandle windowHandle{ input.mWindowHandle };
      const v2i windowSize{ AppWindowApi::GetSize( windowHandle ) };
      const v2i windowPos{ AppWindowApi::GetPos( windowHandle ) };
      const v2 mousePos{ AppKeyboardApi::GetMousePosScreenspace() };
    }
  };

  /*

  enum class MouseEdgeFlags
  {
    kNone      = 0b0001,
    kMovable   = 0b0010,
    kResizable = 0b0100,
  };

  struct MouseEdge
  {
    MouseEdgeFlags    mFlags { MouseEdgeFlags::kNone };
    //DesktopWindowRect mWindowSpaceMoveRect;
    int               mResizeBorder { false };
  };

  enum HandlerType
  {
    None = 0,
    Move,
    Resize,
  };

  // If CursorDir |= CursorDirE, that means we would be dragging the left wide of the window,
  // and the right side of the window would be cursor locked.
  typedef int CursorDir;

  static MouseEdge    sMouseEdges[ kDesktopWindowCapacity ];

  const CursorDir     CursorDirN { 0b0001 };
  const CursorDir     CursorDirW { 0b0010 };
  const CursorDir     CursorDirS { 0b0100 };
  const CursorDir     CursorDirE { 0b1000 };

  static HCURSOR      cursorArrow;
  static HCURSOR      cursorArrowNS;
  static HCURSOR      cursorArrowWE;
  static HCURSOR      cursorArrowNE_SW;
  static HCURSOR      cursorArrowNW_SE;

  // Used to set the cursor icon
  static CursorDir    mCursorLock;
  static POINT        mCursorPositionOnClick;
  static RECT         mWindowRectOnClick;
  static int          edgeDistResizePx;
  static int          edgeDistMovePx;
  static bool         mEverSet;
  static HandlerType  mHandlerType;
  static bool         mIsFinished;
  static HWND         mHwnd;
  static bool         mMouseDownCurr;
  static bool         mMouseDownPrev;
  static GameTime    keyboardMoveT;

  static HWND         sMouseHoveredHwnd;
  static WindowHandle sMouseHoveredDesktopWindow;

  static HCURSOR GetCursor( CursorDir cursorDir )
  {
    // switch by the integral type cuz of the bit-twiddling
    switch( cursorDir )
    {
    case CursorDirN: return cursorArrowNS;
    case CursorDirW: return cursorArrowWE;
    case CursorDirS: return cursorArrowNS;
    case CursorDirE: return cursorArrowWE;
    case CursorDirN | CursorDirE: return cursorArrowNE_SW;
    case CursorDirS | CursorDirW: return cursorArrowNE_SW;
    case CursorDirN | CursorDirW: return cursorArrowNW_SE;
    case CursorDirS | CursorDirE: return cursorArrowNW_SE;
    default: return cursorArrow;
    }
  }

  static const char* CursorDirToString( CursorDir cursorType )
  {
    if( !cursorType )
      return "Default";
    static char result[ 10 ]  {};
    int iResult = 0;
    if( cursorType & CursorDirN ) result[ iResult++ ] = 'N';
    if( cursorType & CursorDirW ) result[ iResult++ ] = 'W';
    if( cursorType & CursorDirS ) result[ iResult++ ] = 'S';
    if( cursorType & CursorDirE ) result[ iResult++ ] = 'E';
    result[ iResult++ ] = '\0';
    return result;
  }

  // care to describe what this function does?
  static void SetCursorLock( CursorDir cursorDir )
  {
    bool verbose { false };
    if( verbose )
      OS::OSDebugPrintLine( ShortFixedString( "Cursor lock: ", cursorDir ) );

    mCursorLock = cursorDir;
  }

  static HWND GetMouseHoveredHWND()
  {
    POINT cursorPos;
    if( !::GetCursorPos( &cursorPos ) )
      return NULL;

    return ::WindowFromPoint( cursorPos );
  }

  static void SetCursor( const CursorDir cursorDir )
  {
    const HCURSOR cursor { GetCursor( cursorDir ) };
    SetCursor( cursor );
  }

  static void UpdateIdle()
  {
#if 0
    if( !sMouseHoveredDesktopWindow.IsValid() )
      return;

    const WindowHandle WindowHandle = sMouseHoveredDesktopWindow;
    const HWND windowHandle = sMouseHoveredHwnd;
    if( !windowHandle )
      return;

    POINT cursorPos;
    if( !GetCursorPos( &cursorPos ) )
      return;

    RECT windowRect;
    if( !GetWindowRect( windowHandle, &windowRect ) )
      return;

    if( cursorPos.x < windowRect.left ||
        cursorPos.x > windowRect.right ||
        cursorPos.y > windowRect.bottom ||
        cursorPos.y < windowRect.top )
      return;

    const int i { WindowHandle.GetIndex() };
    MouseEdge& mouseEdge { sMouseEdges[ i ] };
    const bool mouseEdgeMovable { ( int )mouseEdge.mFlags | ( int )MouseEdgeFlags::kMovable };
    const bool mouseEdgeResizable { ( int )mouseEdge.mFlags | ( int )MouseEdgeFlags::kResizable };

    CursorDir cursorLock  {};
    if( mouseEdgeResizable )
    {
      edgeDistResizePx = mouseEdge.mResizeBorder;
      cursorLock |= cursorPos.x < windowRect.left + edgeDistResizePx ? CursorDirE : 0;
      cursorLock |= cursorPos.x > windowRect.right - edgeDistResizePx ? CursorDirW : 0;
      cursorLock |= cursorPos.y > windowRect.bottom - edgeDistResizePx ? CursorDirN : 0;
      cursorLock |= cursorPos.y < windowRect.top + edgeDistResizePx ? CursorDirS : 0;
    }

    if( mCursorLock != cursorLock || !mEverSet )
    {
      mEverSet = true;
      const HCURSOR cursor { GetCursor( cursorLock ) };
      SetCursor( cursor );
      SetCursorLock( cursorLock );
    }

    if( mMouseDownCurr && !mMouseDownPrev )
    {
      const int moveH { mouseEdge.mWindowSpaceMoveRect.mBottom - mouseEdge.mWindowSpaceMoveRect.mTop };
      const int moveW { mouseEdge.mWindowSpaceMoveRect.mRight - mouseEdge.mWindowSpaceMoveRect.mLeft };
      const int moveL { windowRect.left + mouseEdge.mWindowSpaceMoveRect.mLeft };
      const int moveR { windowRect.left + moveW };
      const int moveB { windowRect.top + moveH };
      const int moveT { windowRect.top + mouseEdge.mWindowSpaceMoveRect.mTop };
      const RECT screenMoveRect  { moveL, moveT, moveR, moveB };
      if( mouseEdgeResizable && cursorLock )
        mHandlerType = HandlerType::Resize;
      else if( mouseEdgeMovable )
      {
        const bool hovered { PtInRect( &screenMoveRect, cursorPos ) };
        if( hovered )
        {
          //Mouse::TryConsumeMouseMovement( &keyboardMoveT, TAC_STACK_FRAME );
          if( keyboardMoveT )
          {
            mHandlerType = HandlerType::Move;
          }
        }
      }
    }

    mCursorPositionOnClick = cursorPos;
    mWindowRectOnClick = windowRect;
    mHwnd = windowHandle;
#endif
  }

  static void UpdateResize()
  {
    if( !mMouseDownCurr )
    {
      mHandlerType = HandlerType::None;
      return;
    }

    //Mouse::TryConsumeMouseMovement( &keyboardMoveT, TAC_STACK_FRAME );

    POINT cursorPos;
    if( !GetCursorPos( &cursorPos ) )
      return;

    LONG dx { cursorPos.x - mCursorPositionOnClick.x };
    LONG dy { cursorPos.y - mCursorPositionOnClick.y };

    RECT rect { mWindowRectOnClick };
    rect.bottom += mCursorLock & CursorDirN ? dy : 0;
    rect.top += mCursorLock & CursorDirS ? dy : 0;
    rect.left += mCursorLock & CursorDirE ? dx : 0;
    rect.right += mCursorLock & CursorDirW ? dx : 0;

    int x { rect.left };
    int y { rect.top };
    int w { rect.right - rect.left };
    int h { rect.bottom - rect.top };
    SetWindowPos( mHwnd, nullptr, x, y, w, h, SWP_ASYNCWINDOWPOS );
  }

  static void UpdateMove()
  {
    if( !mMouseDownCurr )
    {
      mHandlerType = HandlerType::None;
      return;
    }

    //Mouse::TryConsumeMouseMovement( &keyboardMoveT, TAC_STACK_FRAME );

    POINT cursorPos;
    if( !GetCursorPos( &cursorPos ) )
      return;

    int x { mWindowRectOnClick.left + cursorPos.x - mCursorPositionOnClick.x };
    int y { mWindowRectOnClick.top + cursorPos.y - mCursorPositionOnClick.y };
    int w { mWindowRectOnClick.right - mWindowRectOnClick.left };
    int h { mWindowRectOnClick.bottom - mWindowRectOnClick.top };
    SetWindowPos( mHwnd, nullptr, x, y, w, h, SWP_ASYNCWINDOWPOS );
  }
}

void Tac::Win32MouseEdgeInit()
{
  cursorArrow = LoadCursor( NULL, IDC_ARROW );
  cursorArrowNS = LoadCursor( NULL, IDC_SIZENS );
  cursorArrowWE = LoadCursor( NULL, IDC_SIZEWE );
  cursorArrowNE_SW = LoadCursor( NULL, IDC_SIZENESW );
  cursorArrowNW_SE = LoadCursor( NULL, IDC_SIZENWSE );
  edgeDistResizePx = 7;
  edgeDistMovePx = edgeDistResizePx + 6;

  // the distance that your mouse can be from the edge to resize the window is a thin border
  // but the top bar that you can move is a fat border.
  // The resize border sits on top of the move border, so if it were the bigger border it would
  // completely obscure the move border and youd never be able to move your window.
  TAC_ASSERT( edgeDistMovePx > edgeDistResizePx );
}

void Tac::Win32MouseEdgeUpdate()
{
  sMouseHoveredHwnd = GetMouseHoveredHWND();
  sMouseHoveredDesktopWindow = Win32WindowManagerFindWindow( sMouseHoveredHwnd );

  mMouseDownPrev = mMouseDownCurr;
  mMouseDownCurr = GetKeyState( VK_LBUTTON ) & 0x100;
  switch( mHandlerType )
  {
    case Tac::None: UpdateIdle(); break;
    case Tac::Move: UpdateMove(); break;
    case Tac::Resize: UpdateResize(); break;
  }
}

void Tac::Win32MouseEdgeSetMovable( [[maybe_unused]] const WindowHandle& WindowHandle,
                                    [[maybe_unused]] const DesktopWindowRect& windowSpaceRect )
{
#if 0
  const int i { WindowHandle.GetIndex() };
  TAC_ASSERT_INDEX( i, kDesktopWindowCapacity );

  MouseEdge& mouseEdge { sMouseEdges[ i ] };
  mouseEdge.mFlags = MouseEdgeFlags( ( int )mouseEdge.mFlags | ( int )MouseEdgeFlags::kMovable );
  mouseEdge.mWindowSpaceMoveRect = windowSpaceRect;
#endif
}

void Tac::Win32MouseEdgeSetResizable( const WindowHandle& WindowHandle,
                                      int borderPx )
{
  const int i { WindowHandle.GetIndex() };
  TAC_ASSERT_INDEX( i, kDesktopWindowCapacity );
  MouseEdge& mouseEdge { sMouseEdges[i] };
  mouseEdge.mFlags = MouseEdgeFlags( ( int )mouseEdge.mFlags | ( int )MouseEdgeFlags::kResizable );
  mouseEdge.mResizeBorder = borderPx;
}

Tac::WindowHandle Tac::Win32MouseEdgeGetCursorHovered()
{
  return sMouseHoveredDesktopWindow;
}
*/
}
