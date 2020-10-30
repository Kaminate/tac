#include "src/shell/windows/tacWin32MouseEdge.h"
#include "src/common/tacString.h"
#include "src/common/tacPreprocessor.h"

namespace Tac
{
  enum HandlerType
  {
    None,
    Move,
    Resize,
  };


  typedef int CursorDir;
  const CursorDir CursorDirN = 0b0001;
  const CursorDir CursorDirW = 0b0010;
  const CursorDir CursorDirS = 0b0100;
  const CursorDir CursorDirE = 0b1000;

  static HCURSOR cursorArrow;
  static HCURSOR cursorArrowNS;
  static HCURSOR cursorArrowWE;
  static HCURSOR cursorArrowNE_SW;
  static HCURSOR cursorArrowNW_SE;


  // Used to set the cursor icon
  static CursorDir mCursorLock = {};
  static POINT mCursorPositionOnClick = {};
  static RECT mWindowRectOnClick = {};
  static int edgeDistResizePx;
  static int edgeDistMovePx;
  static bool mEverSet = false;
  static HandlerType mHandlerType = HandlerType::None;
  static bool mIsFinished = false;
  static HWND mHwnd = NULL;

  static bool mMouseDownCurr = false;
  static bool mMouseDownPrev = false;

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
    static char result[ 10 ] = {};
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
    bool verbose = false;
    if( verbose )
    {
      std::cout << "Cursor lock: " << CursorDirToString( cursorDir ) << std::endl;;
    }
    mCursorLock = cursorDir;
  }

  static void UpdateIdle( HWND windowHandle )
  {
    if( !windowHandle )
      return;

    POINT cursorPos;
    GetCursorPos( &cursorPos );

    RECT windowRect;
    GetWindowRect( windowHandle, &windowRect );

    if( cursorPos.x < windowRect.left ||
        cursorPos.x > windowRect.right ||
        cursorPos.y > windowRect.bottom ||
        cursorPos.y < windowRect.top )
      return;

    CursorDir cursorLock = {};
    cursorLock |= cursorPos.x < windowRect.left + edgeDistResizePx ? CursorDirE : 0;
    cursorLock |= cursorPos.x > windowRect.right - edgeDistResizePx ? CursorDirW : 0;
    cursorLock |= cursorPos.y > windowRect.bottom - edgeDistResizePx ? CursorDirN : 0;
    cursorLock |= cursorPos.y < windowRect.top + edgeDistResizePx ? CursorDirS : 0;

    if( mCursorLock != cursorLock || !mEverSet )
    {
      mEverSet = true;
      HCURSOR cursor = GetCursor( cursorLock );
      SetCursor( cursor );
      SetCursorLock( cursorLock );
    }
    if( mMouseDownCurr && !mMouseDownPrev )
    {
      if( cursorLock )
        mHandlerType = HandlerType::Resize;
      else if( cursorPos.y < windowRect.top + edgeDistMovePx )
        mHandlerType = HandlerType::Move;
    }

    mCursorPositionOnClick = cursorPos;
    mWindowRectOnClick = windowRect;
    mHwnd = windowHandle;
  }


  static void UpdateResize()
  {
    POINT cursorPos;
    GetCursorPos( &cursorPos );
    LONG dx = cursorPos.x - mCursorPositionOnClick.x;
    LONG dy = cursorPos.y - mCursorPositionOnClick.y;
    RECT rect = mWindowRectOnClick;
    rect.bottom += mCursorLock & CursorDirN ? dy : 0;
    rect.top += mCursorLock & CursorDirS ? dy : 0;
    rect.left += mCursorLock & CursorDirE ? dx : 0;
    rect.right += mCursorLock & CursorDirW ? dx : 0;
    int x = rect.left;
    int y = rect.top;
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    MoveWindow( mHwnd, x, y, w, h, TRUE );
  }

  static void UpdateMove()
  {
    POINT cursorPos;
    GetCursorPos( &cursorPos );
    int x = mWindowRectOnClick.left + cursorPos.x - mCursorPositionOnClick.x;
    int y = mWindowRectOnClick.top + cursorPos.y - mCursorPositionOnClick.y;
    int w = mWindowRectOnClick.right - mWindowRectOnClick.left;
    int h = mWindowRectOnClick.bottom - mWindowRectOnClick.top;
    MoveWindow( mHwnd, x, y, w, h, TRUE );
  }







  void SetCursor( CursorDir cursorDir )
  {
    const HCURSOR cursor = GetCursor( cursorDir );
    SetCursor( cursor );
  }

  void Win32MouseEdgeInit()
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

  void Win32MouseEdgeUpdate( HWND hwnd )
  {
    mMouseDownPrev = mMouseDownCurr;
    mMouseDownCurr = GetKeyState( VK_LBUTTON ) & 0x100;

    if( mHandlerType == HandlerType::None )
    {
      UpdateIdle( hwnd );
    }
    else
    {
      if( !mMouseDownCurr )
        mHandlerType = HandlerType::None;
      if( mHandlerType == HandlerType::Move )
        UpdateMove();
      if( mHandlerType == HandlerType::Resize )
        UpdateResize();
    }

  }



}