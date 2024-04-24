// The prupose of this file is to define the api that the system thread (or platform thread)
// can use to interact with the window system.

#pragma once

#include "tac_window_handle.h"
#include "tac_window_api.h"

// Maybe this shouldn't be done here... the framebuffer texture format should be specified by
// the user somehow... maybe when creating the window? maybe never ( ie in render code )?
// Also imgui can create windows too
#define TAC_WINDOW_BACKEND_CREATES_SWAP_CHAIN() 1

namespace Tac { struct v2i; struct StringView; struct Errors; }
namespace Tac::Render { struct SwapChainHandle; }
namespace Tac
{
  struct SysWindowApi
  {
    bool             IsShown( WindowHandle ) const;
    v2i              GetPos( WindowHandle ) const;
    v2i              GetSize( WindowHandle ) const;
    StringView       GetName( WindowHandle ) const;
    const void*      GetNWH( WindowHandle ) const; // native window handle
    WindowHandle     CreateWindow( WindowCreateParams, Errors& ) const;
    void             DestroyWindow( WindowHandle ) const;

#if TAC_WINDOW_BACKEND_CREATES_SWAP_CHAIN()
    Render::SwapChainHandle GetSwapChainHandle( WindowHandle ) const;
#endif

    void             DesktopWindowDebugImgui();
  };

} // namespace Tac


