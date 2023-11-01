#include "src/common/math/tac_math.h"
#include "src/common/core/tac_algorithm.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/system/tac_os.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/string/tac_string.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/windows/net/tac_net_winsock.h"
#include "src/shell/windows/desktopwindow/tac_win32_desktop_window_manager.h"
#include "src/shell/windows/input/tac_win32_mouse_edge.h"
//#include "src/shell/windows/input/tac_xinput.h"

#include <set>

namespace Tac
{
  static const char*         classname = "tac";

  //                         Elements in this array are added/removed by wndproc
  static HWND                sHWNDs[ kDesktopWindowCapacity ];

  static HWND                mParentHWND = nullptr;
  static DesktopWindowHandle sWindowUnderConstruction;
  static Key                 GetKey( uint8_t keyCode )
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
      case VK_ESCAPE: return Key::Escape;
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


  static LRESULT CALLBACK    WindowProc( const HWND hwnd,
                                      const UINT uMsg,
                                      const WPARAM wParam,
                                      const LPARAM lParam )
  {
    static bool verboseMouseInWindow = false;
    static bool verboseFocus = false;
    static bool verboseActivate = false;
    static bool verboseCapture = false;

    const DesktopWindowHandle desktopWindowHandleFound = Win32WindowManagerFindWindow( hwnd );
    const DesktopWindowHandle desktopWindowHandle
      = desktopWindowHandleFound.IsValid()
      ? desktopWindowHandleFound
      : sWindowUnderConstruction;
    if( !desktopWindowHandle.IsValid() )
    {
      switch( uMsg )
      {
        //case WM_NCACTIVATE: // ??????????????????????
        case WM_NCDESTROY:
          return DefWindowProc( hwnd, uMsg, wParam, lParam );
        default:
          TAC_CRITICAL_ERROR_INVALID_CASE( uMsg );
      }
    }

    switch( uMsg )
    {
      // Sent as a signal that a window or an application should terminate.
      case WM_CLOSE:
      {
        // should it be like...
        // if( window.allow_alt_f4 )
        //    close window?
        //DesktopEventAssignHandle( desktopWindowHandle, nullptr, 0, 0, 0, 0 );
      } break;

      // Sent when a window is being destroyed
      case WM_DESTROY:
      {
        ImGuiSaveWindowSettings();
        sHWNDs[ ( int )desktopWindowHandle ] = nullptr;
        DesktopEventAssignHandle( desktopWindowHandle, nullptr, nullptr, 0, 0, 0, 0 );
      } break;

      case WM_CREATE:
      {
        sHWNDs[ ( int )desktopWindowHandle ] = hwnd;
        sWindowUnderConstruction = DesktopWindowHandle();
        auto windowInfo = ( const CREATESTRUCT* )lParam;
        TAC_ASSERT( windowInfo->cx && windowInfo->cy );
        TAC_ASSERT( windowInfo->lpszName );
        DesktopEventAssignHandle( desktopWindowHandle,
                                  hwnd,
                                  windowInfo->lpszName,
                                  windowInfo->x,
                                  windowInfo->y,
                                  windowInfo->cx,
                                  windowInfo->cy );
      } break;

      // Indicates a request to terminate an application
      case WM_QUIT:
      {
        // mRequestDeletion = true;
      } break;

      // - the window has already been resized ( if you want to resize the window, 
      //   use bgfx WM_USER_WINDOW_SET_SIZE );
      // - notify the logic thread that the windowstate has been updated
      case WM_SIZE:
      {
        const auto width = ( int )LOWORD( lParam );
        const auto height = ( int )HIWORD( lParam );
        DesktopEventResizeWindow( desktopWindowHandle,
                                  width,
                                  height );
      } break;
      case WM_MOVE:
      {
        const auto x = ( int )LOWORD( lParam );
        const auto y = ( int )HIWORD( lParam );
        DesktopEventMoveWindow( desktopWindowHandle,
                                x,
                                y );
      } break;
      case WM_CHAR:
      {
        DesktopEventKeyInput( ( Codepoint )wParam );
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
        DesktopEventKeyState( key, isDown );
      } break;

      //case WM_SETCURSOR:
      //{
      //  result = TRUE;
      //} break;

      case WM_SETFOCUS:
      {
        if( verboseFocus )
          OS::OSDebugPrintLine("window gained keyboard focus ");
      } break;
      case WM_KILLFOCUS:
      {
        if( verboseFocus )
          OS::OSDebugPrintLine("window about to lose keyboard focus" );
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
          OS::OSDebugPrintLine("The window is being activated" );
        }
        else
        {
          if( verboseActivate )
          OS::OSDebugPrintLine("The window is being deactivated" );
        }
      } break;

      case WM_CAPTURECHANGED:
      {
        if( verboseCapture )
          OS::OSDebugPrintLine("WM_CAPTURECHANGED ( mouse capture lost )" );
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
        DesktopEventMouseButtonState( Mouse::Button::MouseLeft, true );

        // make it so clicking the window brings the window to the top of the z order
        SetActiveWindow( hwnd );

        //SetForegroundWindow( mHWND );

      } break;

      case WM_LBUTTONUP:
      {
        DesktopEventMouseButtonState( Mouse::Button::MouseLeft, false );
      } break;

      case WM_RBUTTONDOWN:
      {
        DesktopEventMouseButtonState( Mouse::Button::MouseRight, true );
        //BringWindowToTop( mHWND );
        SetActiveWindow( hwnd ); // make it so clicking the window brings the window to the top of the z order
      } break;
      case WM_RBUTTONUP:
      {
        DesktopEventMouseButtonState( Mouse::Button::MouseRight, false );
      } break;

      case WM_MBUTTONDOWN:
      {
        DesktopEventMouseButtonState( Mouse::Button::MouseMiddle, true );
        //BringWindowToTop( mHWND );

        // make it so clicking the window brings the window to the top of the z order
        SetActiveWindow( hwnd );
      } break;
      case WM_MBUTTONUP:
      {
        DesktopEventMouseButtonState( Mouse::Button::MouseMiddle, false );
      } break;

      case WM_MOUSEMOVE:
      {
        // Allow the window to receive WM_MOUSEMOVE even if the cursor is outside the client area
        // Used for Tac.ImGuiDragFloat
        static HWND mouseTracking;
        if( mouseTracking != hwnd )
        {
          SetCapture( hwnd );
          mouseTracking = hwnd;
        }

        const int xPos = GET_X_LPARAM( lParam );
        const int yPos = GET_Y_LPARAM( lParam );
        DesktopEventMouseMove( desktopWindowHandle, xPos, yPos );
      } break;

      case WM_MOUSEWHEEL:
      {
        const short wheelDeltaParam = GET_WHEEL_DELTA_WPARAM( wParam );
        const short ticks = wheelDeltaParam / WHEEL_DELTA;
        DesktopEventMouseWheel( ( int )ticks );
      } break;

      case WM_MOUSELEAVE:
      {
        //mIsMouseInWindow = false;
        //ReleaseCapture();
        //mCurrDown.clear();
      } break;
    }

    return DefWindowProc( hwnd, uMsg, wParam, lParam );
  }

  static void                RegisterWindowClass( Errors& errors )
  {
    // If you set the cursor here, calls to SetCursor cause it to flicker to the new cursor
    // before reverting back to the old cursor.

    const UINT fuLoad
      = LR_LOADFROMFILE // load a file ( not a resource )
      | LR_DEFAULTSIZE // default metrics based on the type (IMAGE_ICON, 32x32)
      | LR_SHARED;
    const char* iconPath = "assets/grave.ico";
    const auto icon = ( HICON )LoadImage( nullptr, iconPath, IMAGE_ICON, 0, 0, fuLoad );;
    TAC_ASSERT_MSG( icon, "filed to load icon %s", iconPath );

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof( WNDCLASSEX );
    wc.hCursor = nullptr; // LoadCursor( NULL, IDC_ARROW );
    wc.hIcon = icon;
    wc.hIconSm = nullptr; // If null, the system searches for a small icon from the hIcon member
    wc.hInstance = Win32GetStartupInstance();
    wc.hbrBackground = ( HBRUSH )GetStockObject( BLACK_BRUSH );
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = classname;
    wc.style = CS_HREDRAW | CS_VREDRAW; // redraw window on movement or size adjustment

    if( !RegisterClassEx( &wc ) )
    {
      TAC_RAISE_ERROR( "Failed to register window class " + String( classname ), errors  );
    }
  }

  DesktopWindowHandle Win32WindowManagerFindWindow( HWND hwnd )
  {
    //for( int i : WindowHandleIterator() )
    for( int i = 0; i < kDesktopWindowCapacity; ++i )
      if( sHWNDs[ i ] == hwnd )
        return { i };
    return { -1 };
  }


  void                Win32WindowManagerInit( Errors& errors )
  {
    RegisterWindowClass( errors );
  }

  DesktopWindowHandle Win32WindowManagerGetCursorUnobscuredWindow()
  {
    POINT cursorPos;
    const bool cursorPosValid = 0 != ::GetCursorPos( &cursorPos );
    if( !cursorPosValid )
      return {};

    const HWND hoveredHwnd = ::WindowFromPoint( cursorPos );
    const DesktopWindowHandle desktopWindowHandle = Win32WindowManagerFindWindow( hoveredHwnd );
    return desktopWindowHandle;
  }

  void                Win32WindowManagerPoll( Errors& )
  {
    //TAC_PROFILE_BLOCK;
    MSG msg = {};
    while( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }
  }

  void                Win32WindowManagerDespawnWindow( const DesktopWindowHandle& desktopWindowHandle )
  {
    const auto iWindow = ( int )desktopWindowHandle;
    const HWND hwnd = sHWNDs[ iWindow ];

    // unparent the children to prevent them from being
    // sent WM_DESTROY when the parent is destroyed
    // ( doesnt work )
    //if( mParentHWND == hwnd )
    //{
    //  mParentHWND = NULL;
    //  for( HWND child : sHWNDs )
    //  {
    //    if( child && child != hwnd )
    //    {
    //      SetParent( child, NULL );
    //    }
    //  }
    //}

    DestroyWindow( hwnd );
  }

  void                Win32WindowManagerSpawnWindow( const DesktopWindowHandle& desktopWindowHandle,
                                                     const char* name,
                                                     int x,
                                                     int y,
                                                     int w,
                                                     int h )
  {
    const DWORD windowStyle = WS_POPUP;
    if( w && h )
    {
      RECT requestedRect = { 0, 0, w, h };
      if( !AdjustWindowRect( &requestedRect, windowStyle, FALSE ) )
        TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
      w = requestedRect.right - requestedRect.left;
      h = requestedRect.bottom - requestedRect.top;
    }
    else
    {
      const bool isOverlappedWindow =
        ( windowStyle == WS_OVERLAPPED ) ||
        ( ( windowStyle & WS_OVERLAPPEDWINDOW ) == WS_OVERLAPPEDWINDOW );
      if( isOverlappedWindow )
      {
        x = y = w = h = CW_USEDEFAULT;
      }
      else
      {
        int monitorW, monitorH;
        OS::OSGetPrimaryMonitor( &monitorW, &monitorH );
        x = monitorW / 4;
        y = monitorH / 4;
        w = monitorW / 2;
        h = monitorH / 2;
      }
    }

    // if the window is completely outside the desktop area
    // ( this can happen if its saved data refers to a monitor that is no longer on )
    // then move it back inside
    //if( x + w < 0 || x > )
    //{
    //  x = CW_USEDEFAULT;
    //  y = CW_USEDEFAULT;
    //  w = CW_USEDEFAULT;
    //  h = CW_USEDEFAULT;
    //}


    TAC_ASSERT( w && h );

    sWindowUnderConstruction = desktopWindowHandle;
    static HWND parentHWND = nullptr;
    const HWND hwnd = CreateWindow( classname,

                                    // Name of the window, displayed in the window's title bar, or in the 
                                    // alt-tab menu
                                    name,
                                    windowStyle,
                                    x,
                                    y,
                                    w,
                                    h,
                                    parentHWND,
                                    nullptr,
                                    Win32GetStartupInstance(),
                                    nullptr );


    if( !hwnd )
    {
      TAC_CRITICAL_ERROR_INVALID_CODE_PATH;
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
      return;
    }

    parentHWND = hwnd;

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

    ShowWindow( hwnd, Win32GetStartupCmdShow() );
    mParentHWND = mParentHWND ? mParentHWND : hwnd; // combine windows into one tab group
  }

}
