#include "common/tackeyboardinput.h"
#include "common/tacAlgorithm.h"
#include "shell/windows/tacWindowsMouseEdge.h"
#include "shell/windows/tacWindowsApp2.h"

// enum classes amiright
static void operator |= ( TacCursorDir& lhs, TacCursorDir rhs )
{
  lhs = ( TacCursorDir )( ( TacCursorDirType )lhs | ( TacCursorDirType )rhs );
}

TacString ToString( TacCursorDir cursorType )
{
  if( !cursorType )
    return "Default";
  TacString result;
  if( cursorType & TacCursorDir::N ) result += "N";
  if( cursorType & TacCursorDir::W ) result += "W";
  if( cursorType & TacCursorDir::S ) result += "S";
  if( cursorType & TacCursorDir::E ) result += "E";
  return result;
}

TacWin32Cursors::TacWin32Cursors()
{
  cursorArrow = LoadCursor( NULL, IDC_ARROW );
  cursorArrowNS = LoadCursor( NULL, IDC_SIZENS );
  cursorArrowWE = LoadCursor( NULL, IDC_SIZEWE );
  cursorArrowNE_SW = LoadCursor( NULL, IDC_SIZENESW );
  cursorArrowNW_SE = LoadCursor( NULL, IDC_SIZENWSE );
}

HCURSOR TacWin32Cursors::GetCursor( TacCursorDir cursorDir )
{
  // switch by the integral type cuz of the bit-twiddling
  switch( ( TacCursorDirType )cursorDir )
  {
  case TacCursorDir::N: return cursorArrowNS;
  case TacCursorDir::W: return cursorArrowWE;
  case TacCursorDir::S: return cursorArrowNS;
  case TacCursorDir::E: return cursorArrowWE;
  case TacCursorDir::N | TacCursorDir::E: // fallthrough
  case TacCursorDir::S | TacCursorDir::W: return cursorArrowNE_SW;
  case TacCursorDir::N | TacCursorDir::W: // fallthrough
  case TacCursorDir::S | TacCursorDir::E: return cursorArrowNW_SE;
  default: return cursorArrow;
  }
}

BOOL enumfunc( HWND hwndchild, LPARAM userdata )
{
  std::cout << "child " << hwndchild << std::endl;

  return TRUE; // to continue enumerating
}

TacWin32MouseEdgeHandler::TacWin32MouseEdgeHandler()
{
  edgeDistResizePx = 7;
  edgeDistMovePx = edgeDistResizePx + 20;

  // the distance that your mouse can be from the edge to resize the window is a thin border
  // but the top bar that you can move is a fat border.
  // The resize border sits on top of the move border, so if it were the bigger border it would
  // completely obscure the move border and youd never be able to move your window.
  TacAssert( edgeDistMovePx > edgeDistResizePx );
}
void TacWin32MouseEdgeHandler::Update( TacVector< TacWin32DesktopWindow*>& windows )
{
  if( mHandler )
  {
    mHandler->Update();
    if( !mKeyboardInput->IsKeyDown( TacKey::MouseLeft ) )
    {
      mHandler = nullptr;
    }
    return;
  }


  // Brute-forcing the lookup through all the windows, because...
  // - GetTopWindow( parentHwnd ) is returning NULL.
  // - EnumChildWindows( parentHwnd, ... ) also shows that the parent has no children.
  // - Calling GetParent( childHwnd ) returns NULL
  // - Calling GetAncestor( childHwnd, GA_PARENT ) returns
  //   the actual desktop hwnd ( window class #32769 )

  std::set< HWND > ourHwndsNotZSorted;
  for( TacWin32DesktopWindow* window : windows )
    ourHwndsNotZSorted.insert( window->mHWND );

  TacVector< HWND > ourHwndsZSortedTopToBottom;

  HWND topZSortedHwnd = GetTopWindow( NULL );
  int totalWindowCount = 0;
  for( HWND curZSortedHwnd = topZSortedHwnd;
    curZSortedHwnd != NULL;
    curZSortedHwnd = GetWindow( curZSortedHwnd, GW_HWNDNEXT ) )
  {
    totalWindowCount++;
    if( !TacContains( ourHwndsNotZSorted, curZSortedHwnd ) )
      continue;
    ourHwndsNotZSorted.erase( curZSortedHwnd );
    ourHwndsZSortedTopToBottom.push_back( curZSortedHwnd );
  }

  for( HWND windowHandle : ourHwndsZSortedTopToBottom )
  {
    TacString windowName = TacGetWin32WindowName( windowHandle );


    POINT cursorPos;
    GetCursorPos( &cursorPos );

    RECT windowRect;
    GetWindowRect( windowHandle, &windowRect );

    if( cursorPos.x < windowRect.left ||
      cursorPos.x > windowRect.right ||
      cursorPos.y > windowRect.bottom ||
      cursorPos.y < windowRect.top )
      continue;

    TacCursorDir cursorLock = {};
    if( cursorPos.x < windowRect.left + edgeDistResizePx ) cursorLock |= TacCursorDir::E;
    if( cursorPos.x > windowRect.right - edgeDistResizePx ) cursorLock |= TacCursorDir::W;
    if( cursorPos.y > windowRect.bottom - edgeDistResizePx ) cursorLock |= TacCursorDir::N;
    if( cursorPos.y < windowRect.top + edgeDistResizePx ) cursorLock |= TacCursorDir::S;
    if( mCursorLock != cursorLock )
    {
      HCURSOR cursor = mCursors->GetCursor( cursorLock );
      SetCursor( cursor );
      SetCursorLock( cursorLock );
    }
    if( mKeyboardInput->IsKeyJustDown( TacKey::MouseLeft ) )
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

    break;
  }
}
void TacWin32MouseEdgeHandler::ResetCursorLock()
{
  SetCursorLock( {} );
}
void TacWin32MouseEdgeHandler::SetCursorLock( TacCursorDir cursorDir )
{
  bool verbose = false;
  if( verbose )
  {
    std::cout << "Cursor lock: " << ToString( cursorDir ) << std::endl;;
  }
  mCursorLock = cursorDir;
}
void TacWin32MouseEdgeHandler::ResizeHandler::Update()
{
  POINT cursorPos;
  GetCursorPos( &cursorPos );
  LONG dx = cursorPos.x - mHandler->mCursorPositionOnClick.x;
  LONG dy = cursorPos.y - mHandler->mCursorPositionOnClick.y;
  RECT rect = mHandler->mWindowRectOnClick;
  if( mHandler->mCursorLock & TacCursorDir::N ) rect.bottom += dy;
  if( mHandler->mCursorLock & TacCursorDir::S ) rect.top += dy;
  if( mHandler->mCursorLock & TacCursorDir::E ) rect.left += dx;
  if( mHandler->mCursorLock & TacCursorDir::W ) rect.right += dx;
  int x = rect.left;
  int y = rect.top;
  int w = rect.right - rect.left;
  int h = rect.bottom - rect.top;
  MoveWindow( mHwnd, x, y, w, h, TRUE );
}
void TacWin32MouseEdgeHandler::MoveHandler::Update()
{
  POINT cursorPos;
  GetCursorPos( &cursorPos );
  int x = mHandler->mWindowRectOnClick.left + cursorPos.x - mHandler->mCursorPositionOnClick.x;
  int y = mHandler->mWindowRectOnClick.top + cursorPos.y - mHandler->mCursorPositionOnClick.y;
  int w = mHandler->mWindowRectOnClick.right - mHandler->mWindowRectOnClick.left;
  int h = mHandler->mWindowRectOnClick.bottom - mHandler->mWindowRectOnClick.top;
  MoveWindow( mHwnd, x, y, w, h, TRUE );
}




