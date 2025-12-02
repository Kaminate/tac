#pragma once

#include "tac_window_handle.h"
#include "tac_window_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/string/tac_string_view.h"

#undef CreateWindow

namespace Tac
{
  struct AppWindowApi
  {
    static bool IsShown( WindowHandle );
    static bool IsHovered( WindowHandle );
    static auto GetPos( WindowHandle ) -> v2i;
    static void SetPos( WindowHandle, v2i );
    static auto GetSize( WindowHandle ) -> v2i;
    static void SetSize( WindowHandle, v2i );
    static auto GetName( WindowHandle ) -> StringView;
    static auto GetNativeWindowHandle( WindowHandle ) -> const void*;
    static auto CreateWindow( WindowCreateParams, Errors& ) -> WindowHandle;
    static void DestroyWindow( WindowHandle );
    static void MinimizeWindow( WindowHandle );
    static auto GetSwapChainHandle( WindowHandle ) -> Render::SwapChainHandle;
    static void SetSwapChainAutoCreate( bool );
    static void SetSwapChainColorFormat( Render::TexFmt );
    static void SetSwapChainDepthFormat( Render::TexFmt );
    static auto GetSwapChainColorFormat() -> Render::TexFmt;
    static auto GetSwapChainDepthFormat() -> Render::TexFmt;
    static void DesktopWindowDebugImgui();
  };

#if 1
  struct AppWindowMgr
  {
    //enum class Condition
    //{
    //  kNone = 0,
    //  kAlways,
    //  kFirstUse
    //};

    static void SetNextWindowPosition( v2/*, Condition = Condition::kNone*/ );
    static void SetNextWindowSize( v2/*, Condition = Condition::kNone*/ );
    static bool WindowBegin( StringView );
    static void WindowEnd();
    static auto GetWindowHandle() -> WindowHandle;
    static auto GetWindowHandle( StringView ) -> WindowHandle;
    static void FrameBegin();
    static void FrameEnd();
    static void RenderPresent( Errors&);
  };
#endif

} // namespace Tac


