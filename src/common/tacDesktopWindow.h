
#pragma once

#include "tacString.h"
#include "tacEvent.h"

struct TacRendererWindowData;
struct TacRenderView;
struct TacUI2DDrawData;
struct TacUIRoot;
struct TacWindowParams;
struct TacDesktopWindow;
struct TacShell;

struct TacMonitor
{
  int w = 0;
  int h = 0;
};

struct TacWindowParams
{
  TacWindowParams();
  TacString mName;
  int mWidth = 0;
  int mHeight = 0;
  int mX = 0;
  int mY = 0;

  static void GetCenteredPosition( int w, int h, int* x, int* y, TacMonitor );
};


struct TacDesktopWindow : public TacWindowParams
{
  TacDesktopWindow();
  virtual ~TacDesktopWindow();
  void SetRenderViewDefaults();

  // Used to create a vulkan surface
  void* mOperatingSystemApplicationHandle = nullptr;
  void* mOperatingSystemHandle = nullptr;

  bool mRequestDeletion = false;

  // Owned by the renderer
  TacRendererWindowData* mRendererData = nullptr;

  TacRenderView* mRenderView;

  TacEvent<>::Emitter mOnResize;
  TacEvent<>::Emitter mOnMove;
  TacEvent<>::Emitter mOnDestroyed;

  // True if the window directly under the mouse cursor is this one
  // todo: Figure out a better variable name that can be negated
  bool mCursorUnobscured = false;
};

