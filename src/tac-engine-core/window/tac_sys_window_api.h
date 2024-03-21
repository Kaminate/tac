// The prupose of this file is to define the api that the system thread (or platform thread)
// can use to interact with the window system.

#pragma once

#include "tac_window_handle.h"

namespace Tac { struct v2i; struct StringView; }
namespace Tac::Render { struct FBHandle; }
namespace Tac
{
  struct SysWindowApi
  {
    bool             IsShown( WindowHandle ) const;
    v2i              GetPos( WindowHandle ) const;
    v2i              GetSize( WindowHandle ) const;
    StringView       GetName( WindowHandle ) const;
    const void*      GetNWH( WindowHandle ) const; // native window handle
    Render::FBHandle GetFBHandle( WindowHandle ) const;
    void             DesktopWindowDebugImgui();
  };

} // namespace Tac


