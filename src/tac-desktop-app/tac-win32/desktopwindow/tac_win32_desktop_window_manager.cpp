#include "tac_win32_desktop_window_manager.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
//#include "tac-engine-core/hid/tac_keyboard_api.h"
#include "tac-engine-core/profile/tac_profile.h"
//#include "tac-engine-core/settings/tac_settings.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/window/tac_window_backend.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-win32/input/tac_win32_mouse_edge.h"
#include "tac-win32/net/tac_net_winsock.h"

namespace Tac
{
  static const char* sClassName{ "tac" };

  //                  Elements in this array are added/removed by wndproc
  static HWND         sHWNDs[ kDesktopWindowCapacity ]{};
  static HWND         sParentHWND{};
  static WindowHandle sWindowUnderConstruction{};
  static bool         sMouseInWindow{};

  static auto GetKey( u8 keyCode ) -> Key
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

  static auto CALLBACK WindowProc( const HWND hwnd,
                                      const UINT uMsg,
                                      const WPARAM wParam,
                                      const LPARAM lParam ) -> LRESULT
  {
    static bool verboseMouseInWindow{};
    static bool verboseFocus{};
    static bool verboseActivate{};
    static bool verboseCapture{};

    // [ ] Q: wtf is sWindowUnderConstruction???
    const WindowHandle windowHandleFound{ Win32WindowManager::FindWindow( hwnd ) };
    const WindowHandle windowHandle{ windowHandleFound.IsValid()
      ? windowHandleFound
      : sWindowUnderConstruction };
    if( !windowHandle.IsValid() )
    {
      switch( uMsg )
      {
        //case WM_NCACTIVATE: // ??????????????????????
        case WM_NCDESTROY:
          return DefWindowProc( hwnd, uMsg, wParam, lParam );
        default: TAC_ASSERT_INVALID_CASE( uMsg );
      }
    }

    switch( uMsg )
    {
      case WM_SHOWWINDOW:
      {
        const DesktopEventApi::WindowVisibleEvent data
        {
            .mWindowHandle { windowHandle },
            .mVisible {  wParam == TRUE  },
        };
        DesktopEventApi::Queue( data );
      } break;
      // Sent as a signal that a window or an application should terminate.
      case WM_CLOSE:
      {
        // should it be like...
        // if( window.allow_alt_f4 )
        //    close window?
        //DesktopEventAssignHandle( WindowHandle, nullptr, 0, 0, 0, 0 );
      } break;

      // Sent when a window is being destroyed
      case WM_DESTROY:
      {
        ImGuiSaveWindowSettings( windowHandle );
        const int i{ windowHandle.GetIndex() };
        sHWNDs[ i ] = nullptr;
        const DesktopEventApi::WindowDestroyEvent data
        {
          .mWindowHandle = windowHandle
        };
        DesktopEventApi::Queue( data );
      } break;

      case WM_CREATE:
      {
        const int i{ windowHandle.GetIndex() };
        sHWNDs[ i ] = hwnd;
        sWindowUnderConstruction = {};
        auto windowInfo{ ( const CREATESTRUCT* )lParam };
        TAC_ASSERT( windowInfo->cx && windowInfo->cy );
        TAC_ASSERT( windowInfo->lpszName );
        const DesktopEventApi::WindowCreateEvent data
        {
            .mWindowHandle       { windowHandle },
            .mName               { windowInfo->lpszName },
            .mNativeWindowHandle { hwnd },
            .mX                  { windowInfo->x },
            .mY                  { windowInfo->y },
            .mW                  { windowInfo->cx },
            .mH                  { windowInfo->cy },
        };
        DesktopEventApi::Queue( data );
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
        const DesktopEventApi::WindowResizeEvent data
        {
          .mWindowHandle { windowHandle },
          .mWidth        { ( int )LOWORD( lParam ) },
          .mHeight       { ( int )HIWORD( lParam ) },
        };
        DesktopEventApi::Queue( data );
      } break;
      case WM_MOVE:
      {
        const DesktopEventApi::WindowMoveEvent data
        {
          .mWindowHandle { windowHandle },
          .mX            { ( int )LOWORD( lParam ) },
          .mY            { ( int )HIWORD( lParam ) },
        };
        DesktopEventApi::Queue( data );
      } break;
      case WM_CHAR:
      {
        const DesktopEventApi::KeyInputEvent data
        {
          ( Codepoint )wParam
        };
        DesktopEventApi::Queue( data );
      } break;
      case WM_SYSKEYDOWN: // fallthrough
      case WM_SYSKEYUP: // fallthrough
      case WM_KEYDOWN: // fallthrough
      case WM_KEYUP: // fallthrough
      {
        const bool wasDown{ ( lParam & ( ( LPARAM )1 << 30 ) ) != 0 };
        const bool isDown{ ( lParam & ( ( LPARAM )1 << 31 ) ) == 0 };
        if( isDown == wasDown )
          break;

        const Key key{ GetKey( ( u8 )wParam ) };
        if( key == Key::Count )
          break;

        const DesktopEventApi::KeyStateEvent data
        {
          .mKey { key },
          .mDown { isDown },
        };
        DesktopEventApi::Queue( data );
      } break;

      //case WM_SETCURSOR:
      //  return TRUE;

      case WM_SETFOCUS:
      {
        if( verboseFocus )
          OS::OSDebugPrintLine( "window gained keyboard focus " );
      } break;

      case WM_KILLFOCUS:
      {
        if( verboseFocus )
          OS::OSDebugPrintLine( "window about to lose keyboard focus" );
      } break;

      // Sent when a window belonging to a different application than the active window
      // is about to be activated.
      //
      // The message is sent to the application whose window is being activated
      // and to the application whose window is being deactivated.
      case WM_ACTIVATEAPP:
      {
        const DesktopEventApi::WindowActivationEvent::State state{ wParam == TRUE
          ? DesktopEventApi::WindowActivationEvent::Activated
          : DesktopEventApi::WindowActivationEvent::Deactivated };

        const DesktopEventApi::WindowActivationEvent data
        {
          .mWindowHandle { windowHandle },
          .mState        { state },
        };
        DesktopEventApi::Queue( data );

        if( verboseActivate && wParam ) { OS::OSDebugPrintLine( "Window activate!" ); }
        if( verboseActivate && !wParam ) { OS::OSDebugPrintLine( "Window deactivate!" ); }

      } break;

      case WM_CAPTURECHANGED:
      {
        if( verboseCapture )
          OS::OSDebugPrintLine( "WM_CAPTURECHANGED ( mouse capture lost )" );
      } break;


      // this is XBUTTON1 or XBUTTON2 ( side mouse buttons )
      case WM_XBUTTONDOWN:
      {
        SetActiveWindow( hwnd );
      } break;

      case WM_MBUTTONDOWN:
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      {
        Key key{ Key::Count };
        if( uMsg == WM_LBUTTONDOWN ) { key = Key::MouseLeft; }
        if( uMsg == WM_RBUTTONDOWN ) { key = Key::MouseRight; }
        if( uMsg == WM_MBUTTONDOWN ) { key = Key::MouseMiddle; }

        const DesktopEventApi::KeyStateEvent data
        {
          .mKey  { key },
          .mDown { true },
        };
        DesktopEventApi::Queue( data );

        // make it so clicking the window brings the window to the top of the z order
        ::SetActiveWindow( hwnd );

        // Capture mouse input even when the mouse goes out of the client window rect
        if( !::GetCapture() )
          ::SetCapture( hwnd );

      } break;

      case WM_LBUTTONUP:
      case WM_MBUTTONUP:
      case WM_RBUTTONUP:
      {
        Key key{ Key::Count };
        if( uMsg == WM_LBUTTONUP ) { key = Key::MouseLeft; }
        if( uMsg == WM_RBUTTONUP ) { key = Key::MouseRight; }
        if( uMsg == WM_MBUTTONUP ) { key = Key::MouseMiddle; }

        const DesktopEventApi::KeyStateEvent data
        {
          .mKey  { key },
          .mDown {},
        };
        DesktopEventApi::Queue( data );

        // Release mouse input capturing
        if( hwnd == ::GetCapture() )
          ::SetCapture( nullptr );

      } break;

      case WM_MOUSELEAVE:
      {
        sMouseInWindow = false;
      } break;

      case WM_MOUSEMOVE:
      {
        if( !sMouseInWindow )
        {
          sMouseInWindow = true;
          ::SetCursor( ::LoadCursor( nullptr, IDC_ARROW ) );
        }

        const int xPos{ GET_X_LPARAM( lParam ) };
        const int yPos{ GET_Y_LPARAM( lParam ) };

        const DesktopEventApi::MouseMoveEvent data
        {
          .mWindowHandle { windowHandle },
          .mX            { xPos },
          .mY            { yPos },
        };
        DesktopEventApi::Queue( data );
      } break;

      case WM_MOUSEWHEEL:
      {
        const short delta{ GET_WHEEL_DELTA_WPARAM( wParam ) };
        const float deltaScaled{ ( float )delta / WHEEL_DELTA };
        const DesktopEventApi::MouseWheelEvent data{ deltaScaled };
        DesktopEventApi::Queue( data );
      } break;
    }

    return ::DefWindowProc( hwnd, uMsg, wParam, lParam );
  }

  static void RegisterWindowClass( Errors& errors )
  {
    // If you set the cursor here, calls to SetCursor cause it to flicker to the new cursor
    // before reverting back to the old cursor.

    const UINT fuLoad{
      LR_LOADFROMFILE // load a file ( not a resource )
    | LR_DEFAULTSIZE // default metrics based on the type (IMAGE_ICON, 32x32)
    | LR_SHARED };

    const char* iconPath{ "assets/grave.ico" };
    const auto icon{ ( HICON )LoadImage( nullptr, iconPath, IMAGE_ICON, 0, 0, fuLoad ) };;
    TAC_RAISE_ERROR_IF( !icon,
                        ShortFixedString::Concat( "Failed to load icon from: \"", iconPath, "\"" ) );



    // if the hcursor is null, then the window will display a spinning circle when you mouse over
    // your window until you call ::SetCursor
    //
    // However, if hCursor is set so something, such as ::LoadCursor( NULL, IDC_ARROW ), then
    // windows will override calls to ::SetCursor 1 second after you call them
    //
    // microsoft documentation:
    //   A handle to the class cursor.
    //   This member must be a handle to a cursor resource.
    //   If this member is NULL, an application must explicitly set the cursor shape whenever
    //   the mouse moves into the application's window.
    const HCURSOR hCursor{};

    const WNDCLASSEX wc
    {
      .cbSize        { sizeof( WNDCLASSEX ) },
      .style         { CS_HREDRAW | CS_VREDRAW }, // redraw window on movement or size adjustment
      .lpfnWndProc   { WindowProc },
      .hInstance     { Win32GetStartupInstance() },
      .hIcon         { icon },
      .hCursor       { hCursor },
      .hbrBackground { ( HBRUSH )GetStockObject( BLACK_BRUSH ) },
      .lpszClassName { sClassName },
      .hIconSm       {}, // If null, the system searches for a small icon from the hIcon member
    };

    TAC_RAISE_ERROR_IF( !RegisterClassEx( &wc ),
                        "Failed to register window class " + String( sClassName ) );
  }

  auto Win32WindowManager::FindWindow( HWND hwnd ) -> Tac::WindowHandle
  {
    for( int i{}; i < kDesktopWindowCapacity; ++i )
      if( sHWNDs[ i ] == hwnd )
        return { i };
    return {};
  }

  HWND Win32WindowManager::GetHWND( WindowHandle h )
  {
    return sHWNDs[ h.GetIndex() ];
  }

  void Win32WindowManager::DebugImGui()
  {
    if( !ImGuiCollapsingHeader( "Win32WindowManagerDebugImGui" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    int hwndCount{};
    for( int i{}; i < kDesktopWindowCapacity; ++i )
    {
      const HWND hwnd{ sHWNDs[ i ] };
      if( !hwnd )
        continue;

      const ShortFixedString text{ ShortFixedString::Concat( "Window ",
                                                              ToString( i ),
                                                              " has HWND ",
                                                              ToString( ( void* )hwnd ) ) };

      ImGuiText( text );

      hwndCount++;
    }

    if( !hwndCount )
      ImGuiText( "No windows... how are you seeing this?" );
  }

  void Win32WindowManager::Init( Errors& errors )
  {
    RegisterWindowClass( errors );
  }

#if 0
  Tac::WindowHandle   Tac::Win32WindowManagerGetCursorUnobscuredWindow()
  {
    POINT cursorPos;
    const bool cursorPosValid = 0 != ::GetCursorPos( &cursorPos );
    if( !cursorPosValid )
      return {};

    const HWND hoveredHwnd = ::WindowFromPoint( cursorPos );
    return Win32WindowManagerFindWindow( hoveredHwnd );
  }
#endif

  void Win32WindowManager::Poll( Errors& )
  {
    //TAC_PROFILE_BLOCK;
    MSG msg{};
    while( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }
  }

  void Win32WindowManager::DespawnWindow( WindowHandle windowHandle )
  {
    const auto i{ windowHandle.GetIndex() };
    const HWND hwnd{ sHWNDs[ i ] };

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

  void Win32WindowManager::SpawnWindow( const PlatformSpawnWindowParams& params,
                                             Errors& errors )
  {
    const WindowHandle& windowHandle = params.mHandle;

    // Name of the window, displayed in the window's title bar or alt-tab menu
    const char* name{ params.mName };
    int x{ params.mPos.x };
    int y{ params.mPos.y };
    int w{ params.mSize.x };
    int h{ params.mSize.y };

    const DWORD windowStyle{ WS_POPUP };
    if( w && h )
    {
      RECT requestedRect{ 0, 0, w, h };
      if( !AdjustWindowRect( &requestedRect, windowStyle, FALSE ) )
        TAC_ASSERT_INVALID_CODE_PATH;
      w = requestedRect.right - requestedRect.left;
      h = requestedRect.bottom - requestedRect.top;
    }
    else
    {
      const bool isOverlappedWindow{
        ( windowStyle == WS_OVERLAPPED ) ||
        ( ( windowStyle & WS_OVERLAPPEDWINDOW ) == WS_OVERLAPPEDWINDOW ) };
      if( isOverlappedWindow )
      {
        x = y = w = h = CW_USEDEFAULT;
      }
      else
      {
        const v2i monitorSize{ OS::OSGetPrimaryMonitor().mSize };
        const int monitorW{ monitorSize.x };
        const int monitorH{ monitorSize.y };
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

    sWindowUnderConstruction = windowHandle;

    const HINSTANCE hinst{ Win32GetStartupInstance() };
    const HWND hwnd{ CreateWindowA( sClassName,
                                    name,
                                    windowStyle,
                                    x, y, w, h,
                                    sParentHWND,
                                    nullptr,
                                    hinst,
                                    nullptr ) };


    if( !hwnd )
    {

      const String lastErrorString{ Win32GetLastErrorString() };

      String msg;
      msg += "\nhttps://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-createwindowexa";
      msg += "\n - This function typically fails for one of the following reasons";
      msg += "\n - an invalid parameter value";
      msg += "\n - the system class was registered by a different module";
      msg += "\n - The WH_CBT hook is installed and returns a failure code";
      msg += "\n - if one of the controls in the dialog template is not registered, or its window window procedure fails WM_CREATE or WM_NCCREATE";
      msg += "\nGetLastError() returned: ";
      msg += lastErrorString;

      TAC_RAISE_ERROR( msg );
    }


    // Sets the keyboard focus to the specified window
    SetFocus( hwnd );

    SetWindowLong( hwnd, GWL_STYLE, 0 );

    // Brings the thread that created the specified window into the foreground and activates the window.
    // Keyboard input is directed to the window, and various visual cues are changed for the user.
    // The system assigns a slightly higher priority to the thread that created the foreground window
    // than it does to other threads.
    SetForegroundWindow( hwnd );

    ShowWindow( hwnd, Win32GetStartupCmdShow() );
    if( false )
    {
      sParentHWND = sParentHWND ? sParentHWND : hwnd; // combine windows into one tab group
    }
  }

}
