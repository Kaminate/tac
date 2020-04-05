#include "src/common/tacKeyboardinput.h"
#include "src/common/tacAlgorithm.h"
#include "src/shell/windows/tacWindowsMouseEdge.h"
#include "src/shell/windows/tacWindowsApp2.h"

namespace Tac
{


// enum classes amiright
static void operator |= ( CursorDir& lhs, CursorDir rhs )
{
  lhs = ( CursorDir )( ( CursorDirType )lhs | ( CursorDirType )rhs );
}

String ToString( CursorDir cursorType )
{
  if( !cursorType )
    return "Default";
  String result;
  if( cursorType & CursorDir::N ) result += "N";
  if( cursorType & CursorDir::W ) result += "W";
  if( cursorType & CursorDir::S ) result += "S";
  if( cursorType & CursorDir::E ) result += "E";
  return result;
}

Win32Cursors::Win32Cursors()
{
  cursorArrow = LoadCursor( NULL, IDC_ARROW );
  cursorArrowNS = LoadCursor( NULL, IDC_SIZENS );
  cursorArrowWE = LoadCursor( NULL, IDC_SIZEWE );
  cursorArrowNE_SW = LoadCursor( NULL, IDC_SIZENESW );
  cursorArrowNW_SE = LoadCursor( NULL, IDC_SIZENWSE );
}

HCURSOR Win32Cursors::GetCursor( CursorDir cursorDir )
{
  // switch by the integral type cuz of the bit-twiddling
  switch( ( CursorDirType )cursorDir )
  {
  case CursorDir::N: return cursorArrowNS;
  case CursorDir::W: return cursorArrowWE;
  case CursorDir::S: return cursorArrowNS;
  case CursorDir::E: return cursorArrowWE;
  case CursorDir::N | CursorDir::E: // fallthrough
  case CursorDir::S | CursorDir::W: return cursorArrowNE_SW;
  case CursorDir::N | CursorDir::W: // fallthrough
  case CursorDir::S | CursorDir::E: return cursorArrowNW_SE;
  default: return cursorArrow;
  }
}

BOOL enumfunc( HWND hwndchild, LPARAM userdata )
{
  std::cout << "child " << hwndchild << std::endl;

  return TRUE; // to continue enumerating
}

Win32MouseEdgeHandler::Win32MouseEdgeHandler()
{
  edgeDistResizePx = 7;
  edgeDistMovePx = edgeDistResizePx + 6;

  // the distance that your mouse can be from the edge to resize the window is a thin border
  // but the top bar that you can move is a fat border.
  // The resize border sits on top of the move border, so if it were the bigger border it would
  // completely obscure the move border and youd never be able to move your window.
  TAC_ASSERT( edgeDistMovePx > edgeDistResizePx );
}
Win32MouseEdgeHandler::~Win32MouseEdgeHandler()
{
  delete mHandler;
}
void Win32MouseEdgeHandler::Update( Win32DesktopWindow* window )
{
  if( mHandler )
  {
    mHandler->Update();
    if( !KeyboardInput::Instance->IsKeyDown( Key::MouseLeft ) )
    {
      delete mHandler;
      mHandler = nullptr;
    }
    return;
  }

  if( !window )
    return;
  HWND windowHandle = window->mHWND;

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
  if( cursorPos.x < windowRect.left + edgeDistResizePx ) cursorLock |= CursorDir::E;
  if( cursorPos.x > windowRect.right - edgeDistResizePx ) cursorLock |= CursorDir::W;
  if( cursorPos.y > windowRect.bottom - edgeDistResizePx ) cursorLock |= CursorDir::N;
  if( cursorPos.y < windowRect.top + edgeDistResizePx ) cursorLock |= CursorDir::S;
  if( mCursorLock != cursorLock )
  {
    HCURSOR cursor = mCursors->GetCursor( cursorLock );
    SetCursor( cursor );
    SetCursorLock( cursorLock );
  }
  if( KeyboardInput::Instance->IsKeyJustDown( Key::MouseLeft ) )
  {
    if( cursorLock )
      mHandler = new ResizeHandler();
    else if( cursorPos.y < windowRect.top + edgeDistMovePx )
      mHandler = new MoveHandler();
  }

  if( mHandler )
  {
    mCursorPositionOnClick = cursorPos;
    mWindowRectOnClick = windowRect;
    mHandler->mHandler = this;
    mHandler->mHwnd = windowHandle;
  }

}
void Win32MouseEdgeHandler::ResetCursorLock()
{
  SetCursorLock( {} );
}
void Win32MouseEdgeHandler::SetCursorLock( CursorDir cursorDir )
{
  bool verbose = false;
  if( verbose )
  {
    std::cout << "Cursor lock: " << ToString( cursorDir ) << std::endl;;
  }
  mCursorLock = cursorDir;
}
void Win32MouseEdgeHandler::ResizeHandler::Update()
{
  POINT cursorPos;
  GetCursorPos( &cursorPos );
  LONG dx = cursorPos.x - mHandler->mCursorPositionOnClick.x;
  LONG dy = cursorPos.y - mHandler->mCursorPositionOnClick.y;
  RECT rect = mHandler->mWindowRectOnClick;
  if( mHandler->mCursorLock & CursorDir::N ) rect.bottom += dy;
  if( mHandler->mCursorLock & CursorDir::S ) rect.top += dy;
  if( mHandler->mCursorLock & CursorDir::E ) rect.left += dx;
  if( mHandler->mCursorLock & CursorDir::W ) rect.right += dx;
  int x = rect.left;
  int y = rect.top;
  int w = rect.right - rect.left;
  int h = rect.bottom - rect.top;
  MoveWindow( mHwnd, x, y, w, h, TRUE );
}
void Win32MouseEdgeHandler::MoveHandler::Update()
{
  POINT cursorPos;
  GetCursorPos( &cursorPos );
  int x = mHandler->mWindowRectOnClick.left + cursorPos.x - mHandler->mCursorPositionOnClick.x;
  int y = mHandler->mWindowRectOnClick.top + cursorPos.y - mHandler->mCursorPositionOnClick.y;
  int w = mHandler->mWindowRectOnClick.right - mHandler->mWindowRectOnClick.left;
  int h = mHandler->mWindowRectOnClick.bottom - mHandler->mWindowRectOnClick.top;
  MoveWindow( mHwnd, x, y, w, h, TRUE );
}




}
