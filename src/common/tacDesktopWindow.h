#pragma once

#include "src/common/tacString.h"
#include "src/common/tacEvent.h"

namespace Tac
{
  struct RendererWindowData;
  struct RenderView;
  struct UI2DDrawData;
  struct UIRoot;
  struct WindowParams;
  /*struct DesktopWindow*/;
  struct Shell;

  struct Monitor
  {
    int w = 0;
    int h = 0;
  };


  struct WindowParams
  {
    WindowParams();
    String mName;
    int mWidth = 0;
    int mHeight = 0;
    int mX = 0;
    int mY = 0;

    static void GetCenteredPosition( int w, int h, int* x, int* y, Monitor );
  };

  struct DesktopWindowHandle
  {
    int mIndex = -1;
  };

  bool AreWindowHandlesEqual( const DesktopWindowHandle&, const DesktopWindowHandle& );
  bool IsWindowHandleValid( const DesktopWindowHandle& );

  struct DesktopWindowState
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mX = 0;
    int                 mY = 0;
    int                 mWidth = 0;
    int                 mHeight = 0;

    // i dont think this should be here.
    // it should only be used in the main thread, not the stuff thread
    //void*               mNativeWindowHandle = nullptr;
    bool                mCursorUnobscured = false;
  };

  static const int kMaxDesktopWindowStateCount = 10;
  struct DesktopWindowStateCollection
  {
    DesktopWindowState* GetStateAtIndex( int );
    DesktopWindowState* FindDesktopWindowState( DesktopWindowHandle );
    DesktopWindowState mStates[ kMaxDesktopWindowStateCount ];
    static DesktopWindowStateCollection InstanceStuffThread;
  };

  struct DesktopWindow
  {
    virtual void*       GetOperatingSystemHandle() = 0;
    DesktopWindowHandle mHandle; // a handle to uhh ourself
    bool                mCursorUnobscured = false;
    String              mName;
    int                 mWidth = 0;
    int                 mHeight = 0;
    int                 mX = 0;
    int                 mY = 0;
  };
}

