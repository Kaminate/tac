// TODO: The prupose of this file is...

#pragma once

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

  //static const int            kDesktopWindowCapacity = 10;
  //bool                        IsWindowHovered( WindowHandle );
  //void                        SetHoveredWindow( WindowHandle );

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


