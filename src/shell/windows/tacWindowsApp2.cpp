#include "shell/windows/tacWindowsApp2.h"
#include "common/imgui.h"
#include "common/tacPreprocessor.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/tackeyboardinput.h"

#include <cassert>
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
  case VK_UP: return TacKey::Up;
  case VK_LEFT: return TacKey::Left;
  case VK_DOWN: return TacKey::Down;
  case VK_RIGHT: return TacKey::Right;
  case VK_SPACE: return TacKey::Space;
  case 'W': return TacKey::Up;
  case 'A': return TacKey::Left;
  case 'S': return TacKey::Down;
  case 'D': return TacKey::Right;
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

void TacWin32Log::HandleEvent( const TacString& logMessage )
{
  const char* c = logMessage.c_str();
  OutputDebugString( c );
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
    //WindowDebugPrintLine( "gained keyboard focus " );
  } break;

  // Sent to a window immediately before it loses the keyboard focus.
  case WM_KILLFOCUS:
  {
    //mKeyboardInput->mCurrDown.clear();
    //WindowDebugPrintLine( "about to lose keyboard focus" );
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
  case WM_LBUTTONDOWN: { mKeyboardInput->SetIsKeyDown( TacKey::MouseLeft, true ); 
    //SetActiveWindow( mHWND );
    //SetForegroundWindow( mHWND );
  
  } break;
  case WM_LBUTTONUP: {  mKeyboardInput->SetIsKeyDown( TacKey::MouseLeft, false );  } break;

  case WM_RBUTTONDOWN: { mKeyboardInput->SetIsKeyDown( TacKey::MouseRight, true ); } break;
  case WM_RBUTTONUP: {  mKeyboardInput->SetIsKeyDown( TacKey::MouseRight, false );  } break;

  case WM_MBUTTONDOWN: { mKeyboardInput->SetIsKeyDown( TacKey::MouseMiddle, true ); } break;
  case WM_MBUTTONUP: {  mKeyboardInput->SetIsKeyDown( TacKey::MouseMiddle , false );  } break;

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
      //SetCapture( mHWND );
      if( mouseInWindowVerbose )
        std::cout << mName << " mouse enter " << std::endl;
      mIsMouseInWindow = true;
    }
  } break;
  case WM_MOUSEWHEEL:
  {
    //short wheelDeltaParam = GET_WHEEL_DELTA_WPARAM( wParam );
    //short ticks = wheelDeltaParam / WHEEL_DELTA;
    //mMouseWheelRel += ticks;
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
  mShouldWindowHaveBorder = false;
}
TacWindowsApplication2::~TacWindowsApplication2()
{
  gWindowsApplications.erase( this );
}
void TacWindowsApplication2::Init( TacErrors& errors )
{
  RerouteStdOutToOutputDebugString();
  gWindowsApplications.insert( this );



  if( !mShouldWindowHaveBorder )
  {
    // Combine TacWin32MouseEdgeCommonData and TacWin32MouseEdgeHandler?
    mMouseEdgeCommonData = new TacWin32MouseEdgeCommonData();
    mMouseEdgeHandler = new TacWin32MouseEdgeHandler();
    mMouseEdgeHandler->mMouseEdgeCommonData = mMouseEdgeCommonData;
    mMouseEdgeHandler->mKeyboardInput = mShell->mKeyboardInput;
  }


  // If you set the cursor here, calls to SetCursor cause it to flicker to the new cursor
  // before reverting back to the old cursor.
  HCURSOR hCursor = NULL;
  if( mShouldWindowHaveBorder )
    hCursor = LoadCursor( nullptr, IDC_ARROW );

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
}
void TacWindowsApplication2::Poll( TacErrors& errors )
{
  TacKeyboardInput* keyboardInput = mShell->mKeyboardInput;
  keyboardInput->BeforePoll();
  for( TacWin32DesktopWindow* window : mWindows )
  {
    window->Poll( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( mMouseEdgeHandler )
  {
    //mMouseEdgeHandler->Update( mWindows );
    mMouseEdgeHandler->Update( mParentHWND );

    // if the mouse just left the window, reset the cursor lock
    //if( mMouseEdgeHandler && !mMouseEdgeHandler->IsHandling() )
    //  mMouseEdgeHandler->ResetCursorLock();
  }

  if( false )
    keyboardInput->DebugPrintWhenKeysChange();
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
void TacWindowsApplication2::SpawnWindow( const TacWindowParams& windowParams, TacDesktopWindow** desktopWindow, TacErrors& errors )
{
  DWORD windowStyle = mShouldWindowHaveBorder ? WS_OVERLAPPEDWINDOW : WS_POPUP;
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
  createdWindow->mShell = mShell;
  createdWindow->mHWND = hwnd;
  createdWindow->mOperatingSystemHandle = hwnd;

  *( TacWindowParams* )createdWindow = windowParams;
  *desktopWindow = createdWindow;
  mWindows.push_back( createdWindow );

  HWND whatsmyparent = GetParent( hwnd );
  std::cout << windowParams.mName + "'s parent: " << whatsmyparent << std::endl;

  // Used to combine all the windows into one tab group.
  if( !mParentHWND )
    mParentHWND = hwnd;
}

