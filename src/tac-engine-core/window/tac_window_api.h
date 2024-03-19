// TODO: The prupose of this file is...

#pragma once

#include "tac-std-lib/string/tac_string_view.h"

namespace Tac { struct v2i; }
namespace Tac
{
  struct WindowHandle
  {
    WindowHandle( int index = -1 );

    v2i        GetPos() const;
    v2i        GetSize() const;
    StringView GetName() const;
    bool       IsShown() const;
    bool       IsValid() const;
    int        GetIndex() const;

    // [ ] Q: Why would i need this?
    // bool operator ==( const DesktopWindowHandle& ) const = default;
    // bool operator !=( const DesktopWindowHandle& ) const = default;
  private:
    int mIndex;
  };

  //static const int            kDesktopWindowCapacity = 10;
  //bool                        IsWindowHovered( DesktopWindowHandle );
  //void                        SetHoveredWindow( DesktopWindowHandle );

  //void                        DesktopWindowDebugImgui();

  struct WindowApi
  {
    struct CreateParams
    {
      StringView  mName = "";
      int         mX = 0;
      int         mY = 0;
      int         mWidth = 0;
      int         mHeight = 0;
    };

    static WindowHandle CreateWindow( CreateParams );
    static void         DestroyWindow( WindowHandle );
  };

} // namespace Tac


