// The prupose of this file is to define the api that the system thread (or platform thread)
// can use to interact with the window system.

#pragma once

#include "tac_window_handle.h"
#include "tac_window_api.h"
#include "tac-rhi/render3/tac_render_api.h"

#undef CreateWindow

namespace Tac { struct v2i; struct StringView; struct Errors; }
namespace Tac::Render { struct SwapChainHandle; }
namespace Tac
{
  struct SysWindowApi
  {

    bool                    IsShown( WindowHandle ) const;
    v2i                     GetPos( WindowHandle ) const;
    v2i                     GetSize( WindowHandle ) const;
    StringView              GetName( WindowHandle ) const;
    const void*             GetNWH( WindowHandle ) const; // native window handle
    WindowHandle            CreateWindow( WindowCreateParams, Errors& ) const;
    void                    DestroyWindow( WindowHandle ) const;
    Render::SwapChainHandle GetSwapChainHandle( WindowHandle ) const;
    void                    SetSwapChainAutoCreate( bool ) const;
    void                    SetSwapChainColorFormat( Render::TexFmt ) const;
    void                    SetSwapChainDepthFormat( Render::TexFmt ) const;
    Render::TexFmt          GetSwapChainColorFormat() const;
    Render::TexFmt          GetSwapChainDepthFormat() const;

    void                    DesktopWindowDebugImgui();
  };

} // namespace Tac


