// TODO: The prupose of this file is...

#pragma once

#undef CreateWindow

#include "tac-std-lib/string/tac_string_view.h"

namespace Tac { struct v2i; struct v2; }
namespace Tac
{
  struct WindowHandle
  {
    WindowHandle( int index = -1 );

    bool       IsValid() const;
    int        GetIndex() const;

    // Comparison operators so client can compare search among their handles
    // without having to call GetIndex()
    bool operator ==( const WindowHandle& ) const = default;
    bool operator !=( const WindowHandle& ) const = default;

  private:
    int mIndex;
  };

  //void                        DesktopWindowDebugImgui();

  // TODO: make not global and dependency inject
  struct PlatformWindowApi
  {
    bool         IsShown( WindowHandle ) const;
    v2i          GetPos() const;
    v2i          GetSize() const;
    StringView   GetName() const;
  };

  // TODO: make not global and dependency inject
  struct SimulationWindowApi
  {
    struct CreateParams
    {
      StringView  mName = "";
      v2i         mPos;
      v2i         mSize;
    };

    bool         IsShown( WindowHandle ) const;
    v2i          GetPos() const;
    v2i          GetSize() const;
    StringView   GetName() const;

    WindowHandle CreateWindow( CreateParams );
    void         DestroyWindow( WindowHandle );
  };

} // namespace Tac


