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
    static bool                    IsShown( WindowHandle );
    static bool                    IsHovered( WindowHandle );
    static v2i                     GetPos( WindowHandle );
    static void                    SetPos( WindowHandle, v2i );
    static v2i                     GetSize( WindowHandle );
    static void                    SetSize( WindowHandle, v2i );
    static StringView              GetName( WindowHandle );
    static const void*             GetNativeWindowHandle( WindowHandle );
    static WindowHandle            CreateWindow( WindowCreateParams, Errors& );
    static void                    DestroyWindow( WindowHandle );
    static Render::SwapChainHandle GetSwapChainHandle( WindowHandle );
    static void                    SetSwapChainAutoCreate( bool );
    static void                    SetSwapChainColorFormat( Render::TexFmt );
    static void                    SetSwapChainDepthFormat( Render::TexFmt );
    static Render::TexFmt          GetSwapChainColorFormat();
    static Render::TexFmt          GetSwapChainDepthFormat();
    static void                    DesktopWindowDebugImgui();
  };

} // namespace Tac


