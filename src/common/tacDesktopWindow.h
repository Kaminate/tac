#pragma once

#include "src/common/tacString.h"
#include "src/common/tacEvent.h"

namespace Tac
{
  struct RendererWindowData;
  struct RenderView;
  struct UI2DDrawData;
  struct UIRoot;
  //struct WindowParams;
  /*struct DesktopWindow*/;
  struct Shell;

  struct Monitor
  {
    int w = 0;
    int h = 0;
  };


  //struct WindowParams
  //{
  //  WindowParams();
  //  String mName;
  //  int mWidth = 0;
  //  int mHeight = 0;
  //  int mX = 0;
  //  int mY = 0;

  //  void Center();
  //};

  void CenterWindow( int *x, int *y, int w, int h );

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
    void*               mNativeWindowHandle = nullptr;
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

  //struct DesktopWindow
  //{
  //  virtual void*       GetOperatingSystemHandle() = 0;
  //  DesktopWindowHandle mHandle; // a handle to uhh ourself
  //  bool                mCursorUnobscured = false;
  //  String              mName;
  //  int                 mWidth = 0;
  //  int                 mHeight = 0;
  //  int                 mX = 0;
  //  int                 mY = 0;
  //};
}

