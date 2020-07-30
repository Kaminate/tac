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

  const int NullDesktopWindowHandle = -1;
  struct DesktopWindowHandle
  {
    int mIndex = NullDesktopWindowHandle; // hmm
    bool IsValid() const;
    bool operator == ( const DesktopWindowHandle& ) const;
  };

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
  typedef DesktopWindowState DesktopWindowStates[ kMaxDesktopWindowStateCount ];

  struct DesktopWindow : public WindowParams
  {
    DesktopWindow();
    virtual ~DesktopWindow();
    virtual void* GetOperatingSystemHandle() = 0;
    DesktopWindowHandle mHandle; // a handle to uhh ourself
    bool mCursorUnobscured = false;
  };
}

