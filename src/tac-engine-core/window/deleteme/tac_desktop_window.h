#pragma once

#include "tac-std-lib/string/tac_string.h"

namespace Tac { struct v2; }

namespace Tac
{
  struct DesktopWindowState
  {
    v2                        GetPosV2() const;
    v2                        GetSizeV2() const;

    int                       mX { };
    int                       mY { };
    int                       mWidth { };
    int                       mHeight { };
    //const void*               mNativeWindowHandle = nullptr;
    bool                      mShown{ false };
    String                    mName;
  };

  struct DesktopWindowRect
  {
    bool IsEmpty() const;
    int  GetArea() const;
    int  GetWidth() const;
    int  GetHeight() const;

    int  mLeft { };
    int  mRight { };
    int  mBottom { };
    int  mTop { };
  };


  struct DesktopWindowHandle
  {
    DesktopWindowHandle( int index = -1 );

    bool                IsValid() const;
    int                 GetIndex() const;
    DesktopWindowState* GetDesktopWindowState() const;
    const void*         GetDesktopWindowNativeHandle() const;
    DesktopWindowRect   GetDesktopWindowRectScreenspace() const;
    DesktopWindowRect   GetDesktopWindowRectWindowspace() const;

    bool operator ==( const DesktopWindowHandle& ) const = default;
    bool operator !=( const DesktopWindowHandle& ) const = default;

  private:
    int mIndex;
  };

  static const int            kDesktopWindowCapacity = 10;
  bool                        IsWindowHovered( DesktopWindowHandle );
  void                        SetHoveredWindow( DesktopWindowHandle );
  void                        DesktopWindowDebugImgui();
} // namespace Tac

