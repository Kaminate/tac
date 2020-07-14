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
    int mIndex = NullDesktopWindowHandle; // hmm
    bool IsValid();
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
    void SetRenderViewDefaults();

    // Used to create a vulkan surface
    void* mOperatingSystemApplicationHandle = nullptr;

    // Of type HWND for Win32
    void* mOperatingSystemHandle = nullptr;

    bool mRequestDeletion = false;

    RendererWindowData* mRendererData = nullptr;

    //RenderView* mRenderView = nullptr;

    Event<>::Emitter mOnResize;
    Event<>::Emitter mOnMove;
    Event<DesktopWindow*>::Emitter mOnDestroyed;

    DesktopWindowHandle mHandle; // a handle to uhh ourself

    // True if the window directly under the mouse cursor is this one
    // todo: Figure out a better variable name that can be negated
    bool mCursorUnobscured = false;
  };


}

