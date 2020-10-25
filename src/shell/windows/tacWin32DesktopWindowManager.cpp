#include "src/common/math/tacMath.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacSettings.h"
#include "src/common/tacString.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/windows/tacNetWinsock.h"
#include "src/shell/windows/tacWin32DesktopWindowManager.h"
#include "src/shell/windows/tacWin32MouseEdge.h"
#include "src/shell/windows/tacXInput.h"

#include <thread>
#include <iostream>
#include <set>

namespace Tac
{
  static const char* classname = "tac";
  static HWND sHWNDs[ kDesktopWindowCapacity ];
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

  static DesktopWindowHandle FindDesktopWindowHandle( HWND hwnd )
  {
    // this ok?
    for( int i : WindowHandleIterator() )
      if( sHWNDs[ i ] == hwnd )
        return { i };
    return { -1 };
  }

  static LRESULT CALLBACK WindowProc( HWND hwnd,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam )
  {



    // Win32DesktopWindow* window = WindowsApplication2::Instance->FindWin32DesktopWindow( hwnd );
    //if( true )// window )
    //{
    const LRESULT result = 0; // window->HandleWindowProc( uMsg, wParam, lParam );
    static bool verboseMouseInWindow = false;
    static bool verboseFocus = false;
    static bool verboseActivate = false;
    static bool verboseCapture = false;

    const DesktopWindowHandle desktopWindowHandle = FindDesktopWindowHandle( hwnd );
    if( !desktopWindowHandle.IsValid() )
      return DefWindowProc( hwnd, uMsg, wParam, lParam );

    switch( uMsg )
    {
      case WM_CLOSE: // fallthrough
      case WM_DESTROY: // fallthrough
      case WM_QUIT:
      {
        //mRequestDeletion = true;
      } break;

      // - the window has already been resized ( if you want to resize the window, 
      //   use bgfx WM_USER_WINDOW_SET_SIZE );
      // - notify the logic thread that the windowstate has been updated
      case WM_SIZE:
      {
        const int width = ( int )LOWORD( lParam );
        const int height = ( int )HIWORD( lParam );
        DesktopEventQueue::Instance.PushEventResizeWindow( desktopWindowHandle,
                                                           width,
                                                           height );
      } break;
      case WM_MOVE:
      {
        const int x = ( int )LOWORD( lParam );
        const int y = ( int )HIWORD( lParam );
        DesktopEventQueue::Instance.PushEventMoveWindow( desktopWindowHandle,
                                                         x,
                                                         y );
      } break;
      case WM_CHAR:
      {
        DesktopEventQueue::Instance.PushEventKeyInput( ( Codepoint )wParam );
      } break;
      case WM_SYSKEYDOWN: // fallthrough
      case WM_SYSKEYUP: // fallthrough
      case WM_KEYDOWN: // fallthrough
      case WM_KEYUP: // fallthrough
      {
        const bool wasDown = ( lParam & ( ( LPARAM )1 << 30 ) ) != 0;
        const bool isDown = ( lParam & ( ( LPARAM )1 << 31 ) ) == 0;
        if( isDown == wasDown )
          break;
        const Key key = GetKey( ( uint8_t )wParam );
        if( key == Key::Count )
          break;
        DesktopEventQueue::Instance.PushEventKeyState( key, isDown );
      } break;

      case WM_SETFOCUS:
      {
        if( verboseFocus )
          std::cout << "window gained keyboard focus " << std::endl;
      } break;
      case WM_KILLFOCUS:
      {
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
        SetActiveWindow( hwnd );
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
        DesktopEventQueue::Instance.PushEventKeyState( Key::MouseLeft, true );

        // make it so clicking the window brings the window to the top of the z order
        SetActiveWindow( hwnd );

        //SetForegroundWindow( mHWND );

      } break;

      case WM_LBUTTONUP:
      {
        //if( uMsg == WM_LBUTTONUP ) { std::cout << "WM_LBUTTONUP" << std::endl; }
        //else { std::cout << "WM_NCLBUTTONUP" << std::endl; }
        DesktopEventQueue::Instance.PushEventKeyState( Key::MouseLeft, false );
      } break;

      case WM_RBUTTONDOWN:
      {
        DesktopEventQueue::Instance.PushEventKeyState( Key::MouseRight, true );
        //BringWindowToTop( mHWND );
        SetActiveWindow( hwnd ); // make it so clicking the window brings the window to the top of the z order
      } break;
      case WM_RBUTTONUP:
      {
        DesktopEventQueue::Instance.PushEventKeyState( Key::MouseRight, false );
      } break;

      case WM_MBUTTONDOWN:
      {
        DesktopEventQueue::Instance.PushEventKeyState( Key::MouseMiddle, true );
        //BringWindowToTop( mHWND );

        // make it so clicking the window brings the window to the top of the z order
        SetActiveWindow( hwnd );
      } break;
      case WM_MBUTTONUP:
      {
        DesktopEventQueue::Instance.PushEventKeyState( Key::MouseMiddle, false );
      } break;

      case WM_MOUSEMOVE:
      {
        //if( !mIsMouseInWindow )
        //{
        //  TRACKMOUSEEVENT mouseevent = {};
        //  mouseevent.cbSize = sizeof( TRACKMOUSEEVENT );
        //  mouseevent.dwFlags = TME_LEAVE;
        //  mouseevent.hwndTrack = hwnd;
        //  mouseevent.dwHoverTime = HOVER_DEFAULT;
        //  if( 0 == TrackMouseEvent( &mouseevent ) )
        //    mWindowProcErrors = "Track mouse errors: " + GetLastWin32ErrorString();
        //  // Allows this windows to receive mouse-move messages past the edge of the window
        //  //SetCapture( mHWND );
        //  if( verboseCapture )
        //    std::cout << "Setting mouse capture to window" << std::endl;
        //  if( verboseMouseInWindow )
        //    std::cout << mName << " mouse enter " << std::endl;
        //  mIsMouseInWindow = true;
        //}
        const int xPos = ( ( int )( short )LOWORD( lParam ) );
        const int yPos = ( ( int )( short )HIWORD( lParam ) );
        DesktopEventQueue::Instance.PushEventMouseMove( desktopWindowHandle,
                                                        xPos,
                                                        yPos );
      } break;

      case WM_MOUSEWHEEL:
      {
        const short wheelDeltaParam = GET_WHEEL_DELTA_WPARAM( wParam );
        const short ticks = wheelDeltaParam / WHEEL_DELTA;
        DesktopEventQueue::Instance.PushEventMouseWheel( ( int )ticks );
      } break;

      case WM_MOUSELEAVE:
      {
        //if( verboseMouseInWindow )
        //  std::cout << mName << " mouse leave" << std::endl;
        //mIsMouseInWindow = false;
        //ReleaseCapture();
        //mCurrDown.clear();
      } break;
    }
    //return 0;
    if( result )
      return result;
    //}
    return DefWindowProc( hwnd, uMsg, wParam, lParam );
  }

  static void RegisterWindowClass( Errors& errors )
  {
    // If you set the cursor here, calls to SetCursor cause it to flicker to the new cursor
    // before reverting back to the old cursor.

    const UINT fuLoad
      = LR_LOADFROMFILE // load a file ( not a resource )
      | LR_DEFAULTSIZE // default metrics based on the type (IMAGE_ICON, 32x32)
      | LR_SHARED;
    const HICON icon = ( HICON )LoadImage( nullptr, "grave.ico", IMAGE_ICON, 0, 0, fuLoad );;
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof( WNDCLASSEX );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hIcon = icon;
    wc.hIconSm = NULL; // If null, the system searches for a small icon from the hIcon member
    wc.hInstance = ghInstance;
    wc.hbrBackground = ( HBRUSH )GetStockObject( BLACK_BRUSH );
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = classname;
    wc.style = CS_HREDRAW | CS_VREDRAW; // redraw window on movement or size adjustment
    if( !RegisterClassEx( &wc ) )
    {
      errors.mMessage = "Failed to register window class " + String( classname );
      TAC_HANDLE_ERROR( errors );
    }
  }

  void WindowsManagerInit( Errors& errors )
  {
    RegisterWindowClass( errors );
  }

  //Win32DesktopWindow* WindowsApplication2::FindWin32DesktopWindow( HWND hwnd )
  //{
  //  for( Win32DesktopWindow* desktopWindow : mWindows )
  //    if( desktopWindow->mHWND == hwnd )
  //      return desktopWindow;
  //  return nullptr;
  //}


  //Win32DesktopWindow::~Win32DesktopWindow()
  //{
  //  DestroyWindow( mHWND );
  //}


  //void* Win32DesktopWindow::GetOperatingSystemHandle()
  //{

  //  return mHWND;
  //}

  //LRESULT Win32DesktopWindow::HandleWindowProc( UINT uMsg,
  //                                              WPARAM wParam,
  //                                              LPARAM lParam )
  //{
  //}
  //void Win32DesktopWindow::Poll( Errors& errors )
  //{
  //  MSG msg = {};
  //  while( PeekMessage( &msg, mHWND, 0, 0, PM_REMOVE ) )
  //  {
  //    TranslateMessage( &msg );
  //    DispatchMessage( &msg );
  //  }

  //  if( mWindowProcErrors )
  //  {
  //    errors = mWindowProcErrors;
  //    TAC_HANDLE_ERROR( errors );
  //  }
  //}


  DesktopWindowHandle GetCursorUnobscuredWindow2()
  {
    POINT cursorPos;
    const bool cursorPosValid = 0 != ::GetCursorPos( &cursorPos );
    if( !cursorPosValid )
      return DesktopWindowHandle();

    const HWND hoveredHwnd = ::WindowFromPoint( cursorPos );
    const DesktopWindowHandle desktopWindowHandle = FindDesktopWindowHandle( hoveredHwnd );
    return desktopWindowHandle;
  }

  // | each window should have its own copy of mouse xyz to fix this problem?
  // |
  // v
  //Win32DesktopWindow* WindowsApplication2::GetCursorUnobscuredWindow()
  //{
  //  POINT cursorPos;
  //  GetCursorPos( &cursorPos );
  //  // Brute-forcing the lookup through all the windows, because...
  //  // - GetTopWindow( parentHwnd ) is returning NULL.
  //  // - EnumChildWindows( parentHwnd, ... ) also shows that the parent has no children.
  //  // - Calling GetParent( childHwnd ) returns NULL
  //  // - Calling GetAncestor( childHwnd, GA_PARENT ) returns
  //  //   the actual desktop hwnd ( window class #32769 )
  //  HWND topZSortedHwnd = GetTopWindow( NULL );
  //  int totalWindowCount = 0;
  //  for( HWND curZSortedHwnd = topZSortedHwnd;
  //       curZSortedHwnd != NULL;
  //       curZSortedHwnd = GetWindow( curZSortedHwnd, GW_HWNDNEXT ) )
  //  {
  //    totalWindowCount++;
  //    // I tried removing this for loop and just returning an HWND which
  //    // may or may not be a tac HWND. Didn't work.
  //    for( Win32DesktopWindow* window : mWindows )
  //    {
  //      if( window->mHWND != curZSortedHwnd )
  //        continue;
  //      RECT windowRect;
  //      GetWindowRect( curZSortedHwnd, &windowRect );
  //      bool isCursorInside =
  //        cursorPos.x >= windowRect.left &&
  //        cursorPos.x <= windowRect.right &&
  //        cursorPos.y >= windowRect.top &&
  //        cursorPos.y <= windowRect.bottom;
  //      if( isCursorInside )
  //        return window;
  //    }
  //  }
  //  return nullptr;
  //}

  void WindowsManagerPoll( Errors& errors )
  {
    MSG msg = {};
    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }

    //if( mWindowProcErrors )
    //{
    //  errors = mWindowProcErrors;
    //  TAC_HANDLE_ERROR( errors );
    //}

    //for( Win32DesktopWindow* window : mWindows )
    //{
    //  window->Poll( errors );
    //  TAC_HANDLE_ERROR( errors );
    //}

    //Win32DesktopWindow* cursorUnobscuredWindow = GetCursorUnobscuredWindow();
    //for( Win32DesktopWindow* window : mWindows )
    //  window->mCursorUnobscured = cursorUnobscuredWindow == window;

    //DesktopWindowHandle unobscuredDesktopWindowHandle
    //  = cursorUnobscuredWindow
    //  ? cursorUnobscuredWindow->mHandle
    //  : DesktopWindowHandle();
    //if( !AreWindowHandlesEqual( mUnobscuredDesktopWindowHandle, unobscuredDesktopWindowHandle ) )
    //{
    //  DesktopEventQueue::Instance.PushEventCursorUnobscured( unobscuredDesktopWindowHandle );
    //  mUnobscuredDesktopWindowHandle = unobscuredDesktopWindowHandle;
    //}

    //const HWND unobscuredHWND
    //  = cursorUnobscuredWindow
    //  ? cursorUnobscuredWindow->mHWND
    //  : nullptr;
    //Win32MouseEdgeHandler::Instance.Update( unobscuredHWND );
  }

  void WindowsManagerSpawnWindow( const DesktopWindowHandle& desktopWindowHandle,
                                  const int x,
                                  const int y,
                                  const int requestedWidth,
                                  const int requestedHeight )
  {
    DWORD windowStyle = WS_POPUP;
    RECT windowRect = {};
    windowRect.right = requestedWidth;
    windowRect.bottom = requestedHeight;
    if( !AdjustWindowRect( &windowRect, windowStyle, FALSE ) )
    {
      TAC_INVALID_CODE_PATH;
      //errors = "Failed to adjust window rect";
      //TAC_HANDLE_ERROR( errors );
    }

    const int windowAdjustedWidth = windowRect.right - windowRect.left;
    const int windowAdjustedHeight = windowRect.bottom - windowRect.top;

    static HWND mParentHWND = NULL;
    const HWND hwnd = CreateWindow( classname,
                                    "butt", // windowParams.mName.c_str(),
                                    windowStyle,
                                    x, // CW_USEDEFAULT
                                    y, // CW_USEDEFAULT
                                    windowAdjustedWidth,
                                    windowAdjustedHeight,
                                    mParentHWND,
                                    NULL,
                                    ghInstance,
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
    //if( !mShouldWindowHaveBorder )
    // {
    SetWindowLong( hwnd, GWL_STYLE, 0 );
    // }

    // Brings the thread that created the specified window into the foreground and activates the window.
    // Keyboard input is directed to the window, and various visual cues are changed for the user.
    // The system assigns a slightly higher priority to the thread that created the foreground window
    // than it does to other threads.
    SetForegroundWindow( hwnd );

    ShowWindow( hwnd, gnCmdShow );


    sHWNDs[ ( int )desktopWindowHandle ] = hwnd;

    //auto createdWindow = TAC_NEW Win32DesktopWindow;
    //createdWindow->mHWND = hwnd;
    //createdWindow->mHandle = desktopWindowHandle;

    // Used to combine all the windows into one tab group.
    if( mParentHWND == NULL )
      mParentHWND = hwnd;


    DesktopEventQueue::Instance.PushEventAssignHandle( desktopWindowHandle,
                                                       hwnd,
                                                       x,
                                                       y,
                                                       windowAdjustedWidth,
                                                       windowAdjustedHeight );

    // hack
    //DesktopEventQueue::Instance.PushEventMoveWindow( desktopWindowHandle, x, y );
    //DesktopEventQueue::Instance.PushEventResizeWindow( desktopWindowHandle,
    //                                                   windowAdjustedWidth,
    //                                                   windowAdjustedHeight );
    //mWindows.push_back( createdWindow );
    //DesktopApp::SpawnWindow( createdWindow );
  }

  //void WindowsApplication2::RemoveWindow( Win32DesktopWindow*  createdWindow )
  //{
  //  for( int i = 0; i < mWindows.size(); ++i )
  //  {
  //    if( mWindows[ i ] == createdWindow )
  //    {
  //      mWindows[ i ] = mWindows.back();
  //      mWindows.pop_back();
  //      return;
  //    }
  //  }
  //}
}
