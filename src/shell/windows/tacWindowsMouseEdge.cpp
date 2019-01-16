#include "common/tackeyboardinput.h"
#include "shell/windows/tacWindowsMouseEdge.h"

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

TacWin32MouseEdgeCommonData::TacWin32MouseEdgeCommonData()
{
  edgeDistResizePx = 7;
  edgeDistMovePx = edgeDistResizePx + 20;

  // the distance that your mouse can be from the edge to resize the window is a thin border
  // but the top bar that you can move is a fat border.
  // The resize border sits on top of the move border, so if it were the bigger border it would
  // completely obscure the move border and youd never be able to move your window.
  TacAssert( edgeDistMovePx > edgeDistResizePx );

  cursorArrow = LoadCursor( NULL, IDC_ARROW );
  cursorArrowNS = LoadCursor( NULL, IDC_SIZENS );
  cursorArrowWE = LoadCursor( NULL, IDC_SIZEWE );
  cursorArrowNE_SW = LoadCursor( NULL, IDC_SIZENESW );
  cursorArrowNW_SE = LoadCursor( NULL, IDC_SIZENWSE );
}
HCURSOR TacWin32MouseEdgeCommonData::GetCursor( TacCursorDir cursorDir )
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

  //TacVector< HWND > orderedWindows;
  //HWND windowHandle = GetTopWindow( NULL );
  //while( windowHandle )
  //{
  //  orderedWindows.push_back( windowHandle );
  //  windowHandle = GetWindow( windowHandle, GW_HWNDNEXT );
  //}

  for( HWND windowHandle = GetTopWindow( NULL );
    windowHandle;
    windowHandle = GetWindow( windowHandle, GW_HWNDNEXT ) )
  {
    int edgeDistResizePx = mMouseEdgeCommonData->edgeDistResizePx;
    int edgeDistMovePx = mMouseEdgeCommonData->edgeDistMovePx;

    char text[ 100 ];
    GetWindowTextA(
      windowHandle,
      text,
      100
    );

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
      HCURSOR cursor = mMouseEdgeCommonData->GetCursor( cursorLock );
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




