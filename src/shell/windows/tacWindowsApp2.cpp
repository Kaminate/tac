#include "shell/windows/tacWindowsApp2.h"
#include "shell/windows/tacXInput.h"
#include "shell/windows/tacNetWinsock.h"
#include "common/tacSettings.h"
#include "common/tacPreprocessor.h"
#include "common/tacString.h"
#include "common/tacAlgorithm.h"
#include "common/tacErrorHandling.h"
#include "common/tackeyboardinput.h"

#include <thread>
#include <iostream>
#include <set>


const char* classname = "tac";




static void RerouteStdOutToOutputDebugString()
{
  static struct : public std::streambuf
  {
    int overflow( int c ) override
    {
      if( c != EOF )
      {
        char buf[] = { ( char )c, '\0' };
        OutputDebugString( buf );
      }
      return c;
    }
  } outputDebugStringStreamBuf;
  std::cout.rdbuf( &outputDebugStringStreamBuf );
  std::cerr.rdbuf( &outputDebugStringStreamBuf );
  std::clog.rdbuf( &outputDebugStringStreamBuf );
}


static std::set< TacWindowsApplication2* > gWindowsApplications;
static TacWin32DesktopWindow* TacFindWindow( HWND hwnd )
{
  for( TacWindowsApplication2* windowsApplication : gWindowsApplications )
  {
    for( TacWin32DesktopWindow* desktopWindow : windowsApplication->mWindows )
    {
      if( desktopWindow->mHWND == hwnd )
      {
        return desktopWindow;
      }
    }
  }
  return nullptr;
}
static LRESULT CALLBACK WindowProc(
  HWND hwnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam )
{
  //static UINT uPrevMsg;
  //uPrevMsg = uMsg;
  TacWin32DesktopWindow* window = TacFindWindow( hwnd );
  if( window )
  {
    LRESULT result = window->HandleWindowProc( uMsg, wParam, lParam );
    if( result )
      return result;
  }
  return DefWindowProc( hwnd, uMsg, wParam, lParam );
}
static TacKey TacGetKey( uint8_t keyCode )
{
  // List of virtual key codes
  // https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
  switch( keyCode )
  {
    case VK_UP: return TacKey::UpArrow;
    case VK_LEFT: return TacKey::LeftArrow;
    case VK_DOWN: return TacKey::DownArrow;
    case VK_RIGHT: return TacKey::RightArrow;
    case VK_SPACE: return TacKey::Spacebar;
    case VK_DELETE: return TacKey::Delete;
    case VK_BACK: return TacKey::Backspace;
    case 'A': return TacKey::A;
    case 'B': return TacKey::B;
    case 'C': return TacKey::C;
    case 'D': return TacKey::D;
    case 'E': return TacKey::E;
    case 'F': return TacKey::F;
    case 'G': return TacKey::G;
    case 'H': return TacKey::H;
    case 'I': return TacKey::I;
    case 'J': return TacKey::J;
    case 'K': return TacKey::K;
    case 'L': return TacKey::L;
    case 'M': return TacKey::M;
    case 'N': return TacKey::N;
    case 'O': return TacKey::O;
    case 'P': return TacKey::P;
    case 'Q': return TacKey::Q;
    case 'R': return TacKey::R;
    case 'S': return TacKey::S;
    case 'T': return TacKey::T;
    case 'U': return TacKey::U;
    case 'V': return TacKey::V;
    case 'W': return TacKey::W;
    case 'X': return TacKey::X;
    case 'Y': return TacKey::Y;
    case 'Z': return TacKey::Z;
    case VK_OEM_3: return TacKey::Backtick;
    case VK_F1: return TacKey::F1;
    case VK_F2: return TacKey::F2;
    case VK_F3: return TacKey::F3;
    case VK_F4: return TacKey::F4;
    case VK_F5: return TacKey::F5;
    case VK_F6: return TacKey::F6;
    case VK_F7: return TacKey::F7;
    case VK_F8: return TacKey::F8;
    case VK_F9: return TacKey::F9;
    case VK_F10: return TacKey::F10;
    case VK_F11: return TacKey::F11;
    case VK_F12: return TacKey::F12;
    default: return TacKey::Count;
  }
}

TacWin32DesktopWindow::~TacWin32DesktopWindow()
{
  DestroyWindow( mHWND );
}
LRESULT TacWin32DesktopWindow::HandleWindowProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  TacKeyboardInput* mKeyboardInput = app->mShell->mKeyboardInput;
  bool mouseInWindowVerbose = false;

  switch( uMsg )
  {
    case WM_CLOSE:
    case WM_DESTROY:
    case WM_QUIT:
    {
      mRequestDeletion = true;
    } return 0;
    case WM_SIZE:
    {
      mWidth = ( int )LOWORD( lParam );
      mHeight = ( int )HIWORD( lParam );
      mOnResize.EmitEvent();
    } break;
    case WM_MOVE:
    {
      mX = ( int )LOWORD( lParam );
      mY = ( int )HIWORD( lParam );
      mOnMove.EmitEvent();
    } break;
    case WM_CHAR:
    {
      // TODO: convert to unicode
      mKeyboardInput->mWMCharPressedHax = ( char )wParam;
    } break;
    case WM_SYSKEYDOWN: // fallthrough
    case WM_SYSKEYUP: // fallthrough
    case WM_KEYDOWN: // fallthrough
    case WM_KEYUP: // fallthrough
    {
      bool wasDown = ( lParam & ( ( LPARAM )1 << 30 ) ) != 0;
      bool isDown = ( lParam & ( ( LPARAM )1 << 31 ) ) == 0;
      if( isDown == wasDown )
        break;
      TacKey key = TacGetKey( ( uint8_t )wParam );
      if( key == TacKey::Count )
        break;
      mKeyboardInput->SetIsKeyDown( key, isDown );
    } break;
    // Sent to a window after it has gained the keyboard focus.
    case WM_SETFOCUS:
    {
      std::cout << "gained keyboard focus " << std::endl;
    } break;

    // Sent to a window immediately before it loses the keyboard focus.
    case WM_KILLFOCUS:
    {
      //mKeyboardInput->mCurrDown.clear();
      std::cout << "about to lose keyboard focus" << std::endl;
    } break;

    // Sent when a window belonging to a different application than the active window
    // is about to be activated.
    //
    // The message is sent to the application whose window is being activated
    // and to the application whose window is being deactivated.
    case WM_ACTIVATEAPP:
    {
      if( wParam == TRUE )
      {
        // The window is being activated
      }
      else
      {
        // The window is being deactivated
      }
    } break;

    case WM_CAPTURECHANGED:
    {

      std::cout << "WM_CAPTURECHANGED ( mouse capture lost )" << std::endl;
    } break;

    case WM_LBUTTONDOWN:
    {
      std::cout << "WM_LBUTTONDOWN" << std::endl;
      mKeyboardInput->SetIsKeyDown( TacKey::MouseLeft, true );
      //SetActiveWindow( mHWND );
      //SetForegroundWindow( mHWND );

    } break;

    // https://docs.microsoft.com/en-us/windows/desktop/inputdev/wm-nclbuttonup
    // Posted when the user releases the left mouse button while the cursor is
    // within the nonclient area of a window.
    // This message is posted to the window that contains the cursor.
    // If a window has captured the mouse, this message is not posted.
    // case WM_NCLBUTTONUP:
    case WM_LBUTTONUP:
    {
      const char* buttonName =
        uMsg == WM_LBUTTONUP ?
        "WM_LBUTTONUP" :
        "WM_NCLBUTTONUP";
      std::cout << buttonName << std::endl;
      mKeyboardInput->SetIsKeyDown( TacKey::MouseLeft, false );
    } break;

    case WM_RBUTTONDOWN:
    {
      mKeyboardInput->SetIsKeyDown( TacKey::MouseRight, true );
    } break;
    case WM_RBUTTONUP:
    {
      mKeyboardInput->SetIsKeyDown( TacKey::MouseRight, false );
    } break;

    case WM_MBUTTONDOWN:
    {
      mKeyboardInput->SetIsKeyDown( TacKey::MouseMiddle, true );
    } break;
    case WM_MBUTTONUP:
    {
      mKeyboardInput->SetIsKeyDown( TacKey::MouseMiddle, false );
    } break;

    case WM_MOUSEMOVE:
    {
      if( !mIsMouseInWindow )
      {
        TRACKMOUSEEVENT mouseevent = {};
        mouseevent.cbSize = sizeof( TRACKMOUSEEVENT );
        mouseevent.dwFlags = TME_LEAVE;
        mouseevent.hwndTrack = mHWND;
        mouseevent.dwHoverTime = HOVER_DEFAULT;
        if( 0 == TrackMouseEvent( &mouseevent ) )
          mWindowProcErrors = "Track mouse errors: " + TacGetLastWin32ErrorString();

        // Allows this windows to receive mouse-move messages past the edge of the window
        SetCapture( mHWND );

        if( mouseInWindowVerbose )
          std::cout << mName << " mouse enter " << std::endl;
        mIsMouseInWindow = true;
      }
    } break;
    case WM_MOUSEWHEEL:
    {
      short wheelDeltaParam = GET_WHEEL_DELTA_WPARAM( wParam );
      short ticks = wheelDeltaParam / WHEEL_DELTA;
      mKeyboardInput->mCurr.mMouseScroll += ( int )ticks;
    } break;
    case WM_MOUSELEAVE:
    {
      if( mouseInWindowVerbose )
        std::cout << mName << " mouse leave " << std::endl;
      mIsMouseInWindow = false;
      //ReleaseCapture();
      //mCurrDown.clear();
    } break;
  }
  return 0;
}
void TacWin32DesktopWindow::Poll( TacErrors& errors )
{
  MSG msg = {};
  while( PeekMessage( &msg, mHWND, 0, 0, PM_REMOVE ) )
  {
    TranslateMessage( &msg );
    DispatchMessage( &msg );
  }

  if( mWindowProcErrors.size() )
  {
    errors = mWindowProcErrors;
    TAC_HANDLE_ERROR( errors );
  }
}

TacWindowsApplication2::TacWindowsApplication2()
{
}
TacWindowsApplication2::~TacWindowsApplication2()
{
  gWindowsApplications.erase( this );
}
void TacWindowsApplication2::Init( TacErrors& errors )
{
  RerouteStdOutToOutputDebugString();
  gWindowsApplications.insert( this );

}
void TacWindowsApplication2::OnShellInit( TacErrors& errors )
{
  mShell->mControllerInput = new TacXInput( mHInstance, errors );
  TAC_HANDLE_ERROR( errors );

  auto netWinsock = new TacNetWinsock( errors );
  TAC_HANDLE_ERROR( errors );
  netWinsock->mShell = mShell;
  mShell->mNet = netWinsock;

  // window borders should be a higher-level concept, right?
  mShouldWindowHaveBorder = mShell->mSettings->GetBool( nullptr, { "areWindowsBordered" }, false, errors );
  if( !mShouldWindowHaveBorder )
  {
    mMouseEdgeHandler = new TacWin32MouseEdgeHandler();
    mMouseEdgeHandler->mCursors = mCursors;
    mMouseEdgeHandler->mKeyboardInput = mShell->mKeyboardInput;
  }


  // If you set the cursor here, calls to SetCursor cause it to flicker to the new cursor
  // before reverting back to the old cursor.
  HCURSOR hCursor = NULL;
  if( mShouldWindowHaveBorder )
    hCursor = mCursors->cursorArrow;

  UINT fuLoad
    = LR_LOADFROMFILE // load a file ( not a resource )
    | LR_DEFAULTSIZE // default metrics based on the type (IMAGE_ICON, 32x32)
    | LR_SHARED;
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof( WNDCLASSEX );
  wc.style = CS_HREDRAW | CS_VREDRAW; // redraw window on movement or size adjustment
  wc.hIcon = ( HICON )LoadImage( nullptr, "grave.ico", IMAGE_ICON, 0, 0, fuLoad );
  wc.hCursor = hCursor;
  wc.hbrBackground = ( HBRUSH )GetStockObject( BLACK_BRUSH );
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = mHInstance;
  wc.lpszClassName = classname;
  wc.hIconSm = NULL; // If null, the system searches for a small icon from the hIcon member
  if( !RegisterClassEx( &wc ) )
  {
    TacString errorMessage;
    errorMessage += "Failed to register window class ";
    errorMessage += classname;
    errors.mMessage = errorMessage;
    TAC_HANDLE_ERROR( errors );
  }

  // Set the initial cursor, or else the cursor will remain as what it was before
  // the application was launched
  if( hCursor == NULL )
    SetCursor( mCursors->cursorArrow );
}
TacWin32DesktopWindow* TacWindowsApplication2::GetCursorUnobscuredWindow()
{
  POINT cursorPos;
  GetCursorPos( &cursorPos );

  // Brute-forcing the lookup through all the windows, because...
  // - GetTopWindow( parentHwnd ) is returning NULL.
  // - EnumChildWindows( parentHwnd, ... ) also shows that the parent has no children.
  // - Calling GetParent( childHwnd ) returns NULL
  // - Calling GetAncestor( childHwnd, GA_PARENT ) returns
  //   the actual desktop hwnd ( window class #32769 )
  HWND topZSortedHwnd = GetTopWindow( NULL );
  int totalWindowCount = 0;
  for( HWND curZSortedHwnd = topZSortedHwnd;
    curZSortedHwnd != NULL;
    curZSortedHwnd = GetWindow( curZSortedHwnd, GW_HWNDNEXT ) )
  {
    totalWindowCount++;
    // I tried removing this for loop and just returning an HWND which
    // may or may not be a tac HWND. Didn't work.
    for( TacWin32DesktopWindow* window : mWindows )
    {
      if( window->mHWND != curZSortedHwnd )
        continue;

      RECT windowRect;
      GetWindowRect( curZSortedHwnd, &windowRect );

      bool isCursorInside =
        cursorPos.x >= windowRect.left &&
        cursorPos.x <= windowRect.right &&
        cursorPos.y >= windowRect.top &&
        cursorPos.y <= windowRect.bottom;

      if( isCursorInside )
        return window;
    }
  }

  return nullptr;
}
void TacWindowsApplication2::Poll( TacErrors& errors )
{
  TacKeyboardInput* keyboardInput = mShell->mKeyboardInput;
  for( TacWin32DesktopWindow* window : mWindows )
  {
    window->Poll( errors );
    TAC_HANDLE_ERROR( errors );
  }

  TacWin32DesktopWindow* cursorUnobscuredWindow = GetCursorUnobscuredWindow();
  for( TacWin32DesktopWindow* window : mWindows )
    window->mCursorUnobscured = cursorUnobscuredWindow == window;

  if( mMouseEdgeHandler )
  {
    mMouseEdgeHandler->Update( cursorUnobscuredWindow );

    // if the mouse just left the window, reset the cursor lock
    //if( mMouseEdgeHandler && !mMouseEdgeHandler->IsHandling() )
    //  mMouseEdgeHandler->ResetCursorLock();
  }
}
void TacWindowsApplication2::GetPrimaryMonitor( TacMonitor* monitor, TacErrors& errors )
{
  int w = GetSystemMetrics( SM_CXSCREEN );
  int h = GetSystemMetrics( SM_CYSCREEN );
  if( !w || !h )
  {
    errors = "Failed to get monitor dimensions";
    TAC_HANDLE_ERROR( errors );
  }

  monitor->w = w;
  monitor->h = h;
}
void TacWindowsApplication2::SpawnWindowAux( const TacWindowParams& windowParams, TacDesktopWindow** desktopWindow, TacErrors& errors )
{
  DWORD windowStyle = mShouldWindowHaveBorder ? WS_OVERLAPPEDWINDOW : WS_POPUP;
  //windowStyle = WS_OVERLAPPEDWINDOW;
  //if( mParentHWND )
  //  windowStyle = WS_CHILD | WS_BORDER;
  RECT windowRect = {};
  windowRect.right = windowParams.mWidth;
  windowRect.bottom = windowParams.mHeight;
  if( !AdjustWindowRect( &windowRect, windowStyle, FALSE ) )
  {
    errors = "Failed to adjust window rect";
    TAC_HANDLE_ERROR( errors );
  }

  int windowAdjustedWidth = windowRect.right - windowRect.left;
  int windowAdjustedHeight = windowRect.bottom - windowRect.top;

  HINSTANCE hInstance = mHInstance;
  HWND hwnd = CreateWindow(
    classname,
    windowParams.mName.c_str(),
    windowStyle,
    windowParams.mX,
    windowParams.mY,
    windowAdjustedWidth,
    windowAdjustedHeight,
    mParentHWND,
    NULL,
    hInstance,
    NULL );
  if( !hwnd )
  {
    errors += TacJoin( "\n",
      {
        // https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-createwindowexa
        "This function typically fails for one of the following reasons",
        "- an invalid parameter value",
        "- the system class was registered by a different module",
        "- The WH_CBT hook is installed and returns a failure code",
        "- if one of the controls in the dialog template is not registered, or its window window procedure fails WM_CREATE or WM_NCCREATE",
        TacGetLastWin32ErrorString()
      }
    );
    TAC_HANDLE_ERROR( errors );
  }
  // Sets the keyboard focus to the specified window
  SetFocus( hwnd );
  if( !mShouldWindowHaveBorder )
    SetWindowLong( hwnd, GWL_STYLE, 0 );

  // Brings the thread that created the specified window into the foreground and activates the window.
  // Keyboard input is directed to the window, and various visual cues are changed for the user.
  // The system assigns a slightly higher priority to the thread that created the foreground window
  // than it does to other threads.
  SetForegroundWindow( hwnd );

  ShowWindow( hwnd, mNCmdShow );


  auto createdWindow = new TacWin32DesktopWindow();
  createdWindow->app = this;
  createdWindow->mHWND = hwnd;
  createdWindow->mOperatingSystemHandle = hwnd;


  *( TacWindowParams* )createdWindow = windowParams;
  *desktopWindow = createdWindow;
  mWindows.push_back( createdWindow );


  createdWindow->mOnDestroyed.AddCallbackFunctional( [ this, createdWindow ]()
    {
      int i = TacIndexOf( createdWindow, mWindows );
      mWindows[ i ] = mWindows.back();
      mWindows.pop_back();
    } );

  // Used to combine all the windows into one tab group.
  if( mParentHWND == NULL )
    mParentHWND = hwnd;
}

