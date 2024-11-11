#include "tac_win32_platform.h" // self-inc

#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-win32/desktopwindow/tac_win32_desktop_window_manager.h"
#include "tac-win32/input/tac_win32_mouse_edge.h"

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  void Win32PlatformFns::PlatformImGui( Errors& errors ) const
  {
    if( !ImGuiCollapsingHeader( "Win32PlatformFns::PlatformImGui" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    Win32WindowManagerDebugImGui();
  }

  void Win32PlatformFns::PlatformFrameBegin( Errors& errors ) const
  {
    Win32WindowManagerPoll( errors );
  }

  void Win32PlatformFns::PlatformFrameEnd( Errors& errors ) const
  {
    Win32MouseEdgeUpdate();

    const DesktopEventApi::CursorUnobscuredEvent data{ Win32MouseEdgeGetCursorHovered() };
    DesktopEventApi::Queue( data );
  }

  void Win32PlatformFns::PlatformSpawnWindow( const PlatformSpawnWindowParams& params,
                                              Errors& errors ) const
  {
    Win32WindowManagerSpawnWindow( params, errors );
  }

  void Win32PlatformFns::PlatformDespawnWindow( WindowHandle handle ) const
  {
    Win32WindowManagerDespawnWindow( handle );
  }

  void Win32PlatformFns::PlatformSetWindowPos( WindowHandle windowHandle, v2i pos) const
  {
    const HWND hwnd{ Win32WindowManagerGetHWND( windowHandle ) };


    RECT rect;
    if( !::GetWindowRect( hwnd, &rect ) )
    {
      //String errStr{ Win32GetLastErrorString() };
      return;
    }

    const int x{ pos.x };
    const int y{ pos.y };
    const int w{ rect.right - rect.left };
    const int h{ rect.bottom - rect.top };

    ::SetWindowPos( hwnd, nullptr, x, y, w, h, SWP_ASYNCWINDOWPOS );
  }

  void Win32PlatformFns::PlatformSetMouseCursor( PlatformMouseCursor cursor ) const
  {
    static HCURSOR cursorArrow { LoadCursor( NULL, IDC_ARROW ) };
    static HCURSOR cursorArrowNS { LoadCursor( NULL, IDC_SIZENS ) };
    static HCURSOR cursorArrowEW { LoadCursor( NULL, IDC_SIZEWE ) };
    static HCURSOR cursorArrowNE_SW { LoadCursor( NULL, IDC_SIZENESW ) };
    static HCURSOR cursorArrowNW_SE { LoadCursor( NULL, IDC_SIZENWSE ) };

    switch( cursor )
    {
    case PlatformMouseCursor::kNone:        ::SetCursor( nullptr );          break;
    case PlatformMouseCursor::kArrow:       ::SetCursor( cursorArrow );      break;
    case PlatformMouseCursor::kResizeNS:    ::SetCursor( cursorArrowNS );    break;
    case PlatformMouseCursor::kResizeEW:    ::SetCursor( cursorArrowEW );    break;
    case PlatformMouseCursor::kResizeNE_SW: ::SetCursor( cursorArrowNE_SW ); break;
    case PlatformMouseCursor::kResizeNW_SE: ::SetCursor( cursorArrowNW_SE ); break;
    default: TAC_ASSERT_INVALID_CASE( cursor );                              break;
    }
  }

  void Win32PlatformFns::PlatformSetWindowSize( WindowHandle windowHandle, v2i size) const
  {
    const HWND hwnd{ Win32WindowManagerGetHWND( windowHandle ) };


    RECT rect;
    if( !::GetWindowRect( hwnd, &rect ) )
    {
      //String errStr{ Win32GetLastErrorString() };
      return;
    }

    const int x{ rect.left };
    const int y{ rect.top };
    const int w{ size.x };
    const int h{ size.y };

    ::SetWindowPos( hwnd, nullptr, x, y, w, h, SWP_ASYNCWINDOWPOS );
  }

  WindowHandle Win32PlatformFns::PlatformGetMouseHoveredWindow() const
  {
    POINT cursorPos;
    if( !::GetCursorPos( &cursorPos ) )
      return {};

    const HWND hwnd{ ::WindowFromPoint( cursorPos ) };
    return Win32WindowManagerFindWindow( hwnd );
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac
