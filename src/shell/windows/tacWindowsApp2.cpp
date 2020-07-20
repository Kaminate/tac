#include "src/shell/windows/tacWindowsApp2.h"
#include "src/shell/windows/tacXInput.h"
#include "src/shell/windows/tacNetWinsock.h"
//#include "src/shell/tacDesktopEventBackend.h"
#include "src/common/tacSettings.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacString.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"

#include <thread>
#include <iostream>
#include <set>

namespace Tac
{


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


  Win32DesktopWindow* WindowsApplication2::FindWin32DesktopWindow( HWND hwnd )
  {
    for( Win32DesktopWindow* desktopWindow : mWindows )
      if( desktopWindow->mHWND == hwnd )
        return desktopWindow;
    return nullptr;
  }

  Win32DesktopWindow* WindowsApplication2::FindWin32DesktopWindow( DesktopWindowHandle desktopWindowHandle )
  {
    desktopWindowHandle.mIndex;

    TAC_INVALID_CODE_PATH;
    return nullptr;
  }

  static LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
  {
    Win32DesktopWindow* window = WindowsApplication2::Instance->FindWin32DesktopWindow( hwnd );
    if( window )
    {
      const LRESULT result = window->HandleWindowProc( uMsg, wParam, lParam );
      if( result )
        return result;
    }
    return DefWindowProc( hwnd, uMsg, wParam, lParam );
  }
  static Key GetKey( uint8_t keyCode )
  {
    // List of virtual key codes
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx
    switch( keyCode )
    {
      case VK_UP: return Key::UpArrow;
      case VK_LEFT: return Key::LeftArrow;
      case VK_DOWN: return Key::DownArrow;
      case VK_RIGHT: return Key::RightArrow;
      case VK_SPACE: return Key::Spacebar;
      case VK_DELETE: return Key::Delete;
      case VK_BACK: return Key::Backspace;
      case VK_TAB: return Key::Tab;
      case VK_CONTROL: return Key::Modifier;
      case 'A': return Key::A;
      case 'B': return Key::B;
      case 'C': return Key::C;
      case 'D': return Key::D;
      case 'E': return Key::E;
      case 'F': return Key::F;
      case 'G': return Key::G;
      case 'H': return Key::H;
      case 'I': return Key::I;
      case 'J': return Key::J;
      case 'K': return Key::K;
      case 'L': return Key::L;
      case 'M': return Key::M;
      case 'N': return Key::N;
      case 'O': return Key::O;
      case 'P': return Key::P;
      case 'Q': return Key::Q;
      case 'R': return Key::R;
      case 'S': return Key::S;
      case 'T': return Key::T;
      case 'U': return Key::U;
      case 'V': return Key::V;
      case 'W': return Key::W;
      case 'X': return Key::X;
      case 'Y': return Key::Y;
      case 'Z': return Key::Z;
      case VK_OEM_3: return Key::Backtick;
      case VK_F1: return Key::F1;
      case VK_F2: return Key::F2;
      case VK_F3: return Key::F3;
      case VK_F4: return Key::F4;
      case VK_F5: return Key::F5;
      case VK_F6: return Key::F6;
      case VK_F7: return Key::F7;
      case VK_F8: return Key::F8;
      case VK_F9: return Key::F9;
      case VK_F10: return Key::F10;
      case VK_F11: return Key::F11;
      case VK_F12: return Key::F12;
      default: return Key::Count;
    }
  }

  Win32DesktopWindow::~Win32DesktopWindow()
  {
    DestroyWindow( mHWND );
  }
  LRESULT Win32DesktopWindow::HandleWindowProc(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
  {
    static bool verboseMouseInWindow = false;
    static bool verboseFocus = false;
    static bool verboseActivate = false;
    static bool verboseCapture = false;

    switch( uMsg )
    {
      case WM_CLOSE: // fallthrough
      case WM_DESTROY: // fallthrough
      case WM_QUIT:
      {
        mRequestDeletion = true;
      } return 0;
      case WM_SIZE:
      {
        mWidth = ( int )LOWORD( lParam );
        mHeight = ( int )HIWORD( lParam );
        mOnResize.EmitEvent();
        DesktopEvent::PushEventResizeWindow( mHandle, mWidth, mHeight );
      } break;
      case WM_MOVE:
      {
        mX = ( int )LOWORD( lParam );
        mY = ( int )HIWORD( lParam );
        mOnMove.EmitEvent();
        DesktopEvent::PushEventMoveWindow( mHandle, mX, mY );
      } break;
      case WM_CHAR:
      {
        // TODO: convert to unicode
        KeyboardInput::Instance->mWMCharPressedHax = ( char )wParam;
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
        Key key = GetKey( ( uint8_t )wParam );
        if( key == Key::Count )
          break;
        KeyboardInput::Instance->SetIsKeyDown( key, isDown );
      } break;

      case WM_SETFOCUS:
      {
        if( verboseFocus )
          std::cout << "window gained keyboard focus " << std::endl;
      } break;
      case WM_KILLFOCUS:
      {
        //KeyboardInput::Instance->mCurrDown.clear();
        if( verboseFocus )
          std::cout << "window about to lose keyboard focus" << std::endl;
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
          if( verboseActivate )
            std::cout << "The window is being activated" << std::endl;
        }
        else
        {
          if( verboseActivate )
            std::cout << "The window is being deactivated" << std::endl;
        }
      } break;

      case WM_CAPTURECHANGED:
      {
        if( verboseCapture )
          std::cout << "WM_CAPTURECHANGED ( mouse capture lost )" << std::endl;
      } break;


      // this is XBUTTON1 or XBUTTON2 ( side mouse buttons )
      case WM_XBUTTONDOWN:
      {
        SetActiveWindow( mHWND );
      } break;


      // https://docs.microsoft.com/en-us/windows/desktop/inputdev/wm-nclbuttonup
      // Posted when the user releases the left mouse button while the cursor is
      // within the nonclient area of a window.
      // This message is posted to the window that contains the cursor.
      // If a window has captured the mouse, this message is not posted.
      // case WM_NCLBUTTONUP:

      case WM_LBUTTONDOWN:
      {
        //std::cout << "WM_LBUTTONDOWN" << std::endl;
        KeyboardInput::Instance->SetIsKeyDown( Key::MouseLeft, true );
        SetActiveWindow( mHWND ); // make it so clicking the window brings the window to the top of the z order
        //SetForegroundWindow( mHWND );

      } break;

      case WM_LBUTTONUP:
      {
        //if( uMsg == WM_LBUTTONUP ) { std::cout << "WM_LBUTTONUP" << std::endl; }
        //else { std::cout << "WM_NCLBUTTONUP" << std::endl; }
        KeyboardInput::Instance->SetIsKeyDown( Key::MouseLeft, false );
      } break;

      case WM_RBUTTONDOWN:
      {
        KeyboardInput::Instance->SetIsKeyDown( Key::MouseRight, true );
        //BringWindowToTop( mHWND );
        SetActiveWindow( mHWND ); // make it so clicking the window brings the window to the top of the z order
      } break;
      case WM_RBUTTONUP:
      {
        KeyboardInput::Instance->SetIsKeyDown( Key::MouseRight, false );
      } break;

      case WM_MBUTTONDOWN:
      {
        KeyboardInput::Instance->SetIsKeyDown( Key::MouseMiddle, true );
        //BringWindowToTop( mHWND );
        SetActiveWindow( mHWND ); // make it so clicking the window brings the window to the top of the z order
      } break;
      case WM_MBUTTONUP:
      {
        KeyboardInput::Instance->SetIsKeyDown( Key::MouseMiddle, false );
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
            mWindowProcErrors = "Track mouse errors: " + GetLastWin32ErrorString();

          // Allows this windows to receive mouse-move messages past the edge of the window
          SetCapture( mHWND );
          if( verboseCapture )
            std::cout << "Setting mouse capture to window" << std::endl;


          if( verboseMouseInWindow )
            std::cout << mName << " mouse enter " << std::endl;
          mIsMouseInWindow = true;
        }
      } break;

      case WM_MOUSEWHEEL:
      {
        short wheelDeltaParam = GET_WHEEL_DELTA_WPARAM( wParam );
        short ticks = wheelDeltaParam / WHEEL_DELTA;
        KeyboardInput::Instance->mCurr.mMouseScroll += ( int )ticks;
      } break;

      case WM_MOUSELEAVE:
      {
        if( verboseMouseInWindow )
          std::cout << mName << " mouse leave" << std::endl;
        mIsMouseInWindow = false;
        //ReleaseCapture();
        //mCurrDown.clear();
      } break;
    }
    return 0;
  }
  void Win32DesktopWindow::Poll( Errors& errors )
  {
    MSG msg = {};
    while( PeekMessage( &msg, mHWND, 0, 0, PM_REMOVE ) )
    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }

    if( mWindowProcErrors )
    {
      errors = mWindowProcErrors;
      TAC_HANDLE_ERROR( errors );
    }
  }

  WindowsApplication2* WindowsApplication2::Instance;
  WindowsApplication2::WindowsApplication2()
  {
    Instance = this;
  }
  WindowsApplication2::WindowsApplication2(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow ) : WindowsApplication2()
  {
    mHInstance = hInstance;
    mlpCmdLine = lpCmdLine;
    mNCmdShow = nCmdShow;
    mhPrevInstance = hPrevInstance;
  }
  WindowsApplication2::~WindowsApplication2()
  {
    for( const Errors& errors : { mErrorsMainThread, mErrorsStuffThread } )
      if( errors )
        OS::DebugPopupBox( errors.ToString() );
  }

  void WindowsApplication2::CreateControllerInput( Errors&errors )
  {
    TAC_NEW XInput( mHInstance, errors );
  }
  void WindowsApplication2::Init( Errors& errors )
  {

    DesktopApp::Init( errors );
    TAC_HANDLE_ERROR( errors );

    RerouteStdOutToOutputDebugString();

    TAC_NEW Win32Cursors;
    TAC_NEW NetWinsock( errors );
    TAC_HANDLE_ERROR( errors );

    // window borders should be a higher-level concept, right?
    mShouldWindowHaveBorder = Shell::Instance->mSettings->GetBool( nullptr, { "areWindowsBordered" }, false, errors );
    if( !mShouldWindowHaveBorder )
    {
      mMouseEdgeHandler = TAC_NEW Win32MouseEdgeHandler();
    }


    // If you set the cursor here, calls to SetCursor cause it to flicker to the new cursor
    // before reverting back to the old cursor.

    UINT fuLoad
      = LR_LOADFROMFILE // load a file ( not a resource )
      | LR_DEFAULTSIZE // default metrics based on the type (IMAGE_ICON, 32x32)
      | LR_SHARED;
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof( WNDCLASSEX );
    wc.style = CS_HREDRAW | CS_VREDRAW; // redraw window on movement or size adjustment
    wc.hIcon = ( HICON )LoadImage( nullptr, "grave.ico", IMAGE_ICON, 0, 0, fuLoad );
    wc.hCursor = mShouldWindowHaveBorder ? Win32Cursors::Instance->cursorArrow : nullptr;
    wc.hbrBackground = ( HBRUSH )GetStockObject( BLACK_BRUSH );
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = mHInstance;
    wc.lpszClassName = classname;
    wc.hIconSm = NULL; // If null, the system searches for a small icon from the hIcon member
    if( !RegisterClassEx( &wc ) )
    {
      errors.mMessage = "Failed to register window class " + String( classname );
      TAC_HANDLE_ERROR( errors );
    }

  }
  Win32DesktopWindow* WindowsApplication2::GetCursorUnobscuredWindow()
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
      for( Win32DesktopWindow* window : mWindows )
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
  void WindowsApplication2::Poll( Errors& errors )
  {
    for( Win32DesktopWindow* window : mWindows )
    {
      window->Poll( errors );
      TAC_HANDLE_ERROR( errors );
    }

    Win32DesktopWindow* cursorUnobscuredWindow = GetCursorUnobscuredWindow();
    for( Win32DesktopWindow* window : mWindows )
      window->mCursorUnobscured = cursorUnobscuredWindow == window;

    // uhh, for now, just close everything when the user closes a window.
    // this is so that we can kill from renderdoc
    for( Win32DesktopWindow* window : mWindows )
      if( window->mRequestDeletion )
        OS::mShouldStopRunning = true;


    DesktopWindowHandle unobscuredDesktopWindowHandle;
    if( cursorUnobscuredWindow )
      unobscuredDesktopWindowHandle = cursorUnobscuredWindow->mHandle;
    DesktopEvent::PushEventCursorUnobscured( unobscuredDesktopWindowHandle );

    if( mMouseEdgeHandler )
    {
      mMouseEdgeHandler->Update( cursorUnobscuredWindow );

      // if the mouse just left the window, reset the cursor lock
      //if( mMouseEdgeHandler && !mMouseEdgeHandler->IsHandling() )
      //  mMouseEdgeHandler->ResetCursorLock();
    }
  }
  void WindowsApplication2::GetPrimaryMonitor( Monitor* monitor, Errors& errors )
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
  void WindowsApplication2::SpawnWindow( DesktopWindowHandle handle,
                                         int x,
                                         int y,
                                         int width,
                                         int height )
  {
    DWORD windowStyle = mShouldWindowHaveBorder ? WS_OVERLAPPEDWINDOW : WS_POPUP;
    //windowStyle = WS_OVERLAPPEDWINDOW;
    //if( mParentHWND )
    //  windowStyle = WS_CHILD | WS_BORDER;
    RECT windowRect = {};
    windowRect.right = width;
    windowRect.bottom = height;
    if( !AdjustWindowRect( &windowRect, windowStyle, FALSE ) )
    {
      TAC_INVALID_CODE_PATH;
      //errors = "Failed to adjust window rect";
      //TAC_HANDLE_ERROR( errors );
    }

    int windowAdjustedWidth = windowRect.right - windowRect.left;
    int windowAdjustedHeight = windowRect.bottom - windowRect.top;

    HINSTANCE hInstance = mHInstance;
    HWND hwnd = CreateWindow(
      classname,
      "butt", // windowParams.mName.c_str(),
      windowStyle,
      x, // CW_USEDEFAULT
      y, // CW_USEDEFAULT
      windowAdjustedWidth,
      windowAdjustedHeight,
      mParentHWND,
      NULL,
      hInstance,
      NULL );
    if( !hwnd )
    {
      TAC_INVALID_CODE_PATH;
      //errors += Join( "\n",
      //  {
      //    // https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-createwindowexa
      //    "This function typically fails for one of the following reasons",
      //    "- an invalid parameter value",
      //    "- the system class was registered by a different module",
      //    "- The WH_CBT hook is installed and returns a failure code",
      //    "- if one of the controls in the dialog template is not registered, or its window window procedure fails WM_CREATE or WM_NCCREATE",
      //    GetLastWin32ErrorString()
      //  }
      //);
      //TAC_HANDLE_ERROR( errors );
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


    auto createdWindow = TAC_NEW Win32DesktopWindow();
    createdWindow->mHWND = hwnd;
    createdWindow->mOperatingSystemHandle = hwnd;
    createdWindow->mOnDestroyed.AddCallbackFunctional(
      []( DesktopWindow* desktopWindow )
      {
        WindowsApplication2::Instance->RemoveWindow( ( Win32DesktopWindow* )desktopWindow );
      } );
    createdWindow->mHandle = handle;

    // Used to combine all the windows into one tab group.
    if( mParentHWND == NULL )
      mParentHWND = hwnd;


    DesktopEvent::PushEventCreateWindow( handle, width, height, x, y, hwnd );
    mWindows.push_back( createdWindow );
    DesktopApp::SpawnWindow( createdWindow );
  }
  void WindowsApplication2::RemoveWindow( Win32DesktopWindow*  createdWindow )
  {
    for( int i = 0; i < mWindows.size(); ++i )
    {
      if( mWindows[ i ] == createdWindow )
      {
        mWindows[ i ] = mWindows.back();
        mWindows.pop_back();
        return;
      }
    }
  }
}
