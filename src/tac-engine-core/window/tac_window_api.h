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

    // int api
    v2i        GetPosi() const;
    v2i        GetSizei() const;
    int        GetX() const;
    int        GetY() const;
    int        GetWidth() const;
    int        GetHeight() const;

    // float api
    v2         GetPosf() const;
    v2         GetSizef() const;

    StringView GetName() const;
    bool       IsShown() const;
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
  };

  // TODO: make not global and dependency inject
  struct SimulationWindowApi
  {
    struct CreateParams
    {
      StringView  mName = "";
      int         mX = 0;
      int         mY = 0;
      int         mWidth = 0;
      int         mHeight = 0;
    };

    WindowHandle CreateWindow( CreateParams );
    void         DestroyWindow( WindowHandle );
  };

} // namespace Tac


