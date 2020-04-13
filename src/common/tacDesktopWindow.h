

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
  struct DesktopWindow;
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

  const int NullDesktopWindowHandle = - 1;
  struct DesktopWindowHandle
  {
    int mIndex = NullDesktopWindowHandle;
  };

	struct DesktopWindowState
	{
		DesktopWindowHandle mDesktopWindowHandle;
		int                 mWidth = 0;
    int                 mHeight = 0;
    void*               mNativeWindowHandle = nullptr;
	};

  struct DesktopWindow : public WindowParams
  {
    DesktopWindow();
    virtual ~DesktopWindow();
    void SetRenderViewDefaults();

    // Used to create a vulkan surface
    void* mOperatingSystemApplicationHandle = nullptr;
    void* mOperatingSystemHandle = nullptr;

    bool mRequestDeletion = false;

    RendererWindowData* mRendererData = nullptr;

    RenderView* mRenderView = nullptr;

    Event<>::Emitter mOnResize;
    Event<>::Emitter mOnMove;
    Event<DesktopWindow*>::Emitter mOnDestroyed;

    // True if the window directly under the mouse cursor is this one
    // todo: Figure out a better variable name that can be negated
    bool mCursorUnobscured = false;
  };


}

