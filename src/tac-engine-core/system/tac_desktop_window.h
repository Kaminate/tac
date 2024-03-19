#pragma once

//#include "tac-std-lib/identifier/tac_handle.h"
#include "tac-std-lib/string/tac_string.h"
//#include "tac-std-lib/tac_core.h"

namespace Tac
{
  struct v2;

  struct DesktopWindowState
  {
    v2                        GetPosV2() const;
    v2                        GetSizeV2() const;

    int                       mX = 0;
    int                       mY = 0;
    int                       mWidth = 0;
    int                       mHeight = 0;
    const void*               mNativeWindowHandle = nullptr;
    String                    mName;
  };

  struct DesktopWindowRect
  {
    bool IsEmpty() const;
    int  GetArea() const;
    int  GetWidth() const;
    int  GetHeight() const;

    int  mLeft = 0;
    int  mRight = 0;
    int  mBottom = 0;
    int  mTop = 0;
  };


  struct DesktopWindowHandle
  {
    DesktopWindowHandle( int index = -1 ) : mIndex{ index } {}

    //explicit operator int() const { return mIndex; }
    //explicit operator unsigned() const { return ( unsigned )mIndex; }

    bool operator ==( const DesktopWindowHandle& ) const = default;
    bool operator !=( const DesktopWindowHandle& ) const = default;

    bool              IsValid() const { return mIndex != -1; }
    int               GetIndex() const { return mIndex; }

    DesktopWindowState* GetDesktopWindowState() const;
    const void*         GetDesktopWindowNativeHandle() const;
    DesktopWindowRect   GetDesktopWindowRectScreenspace() const;
    DesktopWindowRect   GetDesktopWindowRectWindowspace() const;

  private:
    int               mIndex;
  };

  static const int            kDesktopWindowCapacity = 10;
  bool                        IsWindowHovered( DesktopWindowHandle );
  void                        SetHoveredWindow( DesktopWindowHandle );
  void                        DesktopWindowDebugImgui();
} // namespace Tac

