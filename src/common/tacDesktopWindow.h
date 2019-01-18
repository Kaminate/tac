
#pragma once

#include "tacString.h"
#include "tacUtility.h"
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

  TacEvent<>::Emitter mOnDestroyed;

  // Used to create a vulkan surface...
  void* mOperatingSystemApplicationHandle = nullptr;
  void* mOperatingSystemHandle = nullptr;

  bool mRequestDeletion = false;
  TacRendererWindowData* mRendererData = nullptr;
  TacRenderView* mMainWindowRenderView = nullptr;

  // Should this be here? a window doesn't necessarily need ui
  //TacUI2DDrawData* mUI2DDrawData = nullptr;

  // TODO: comment why this is here
  TacShell* mShell = nullptr;

  // Should this be here?
  // A window doesn't necessarily have to have ui
  //TacUIRoot* mUIRoot = nullptr;
  TacEvent<>::Emitter mOnResize;
  TacEvent<>::Emitter mOnMove;
};

