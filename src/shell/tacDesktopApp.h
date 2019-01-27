#pragma once

#include "common/tacUtility.h"
#include "common/tacShell.h"
#include "common/tacErrorHandling.h"
#include "common/tacDesktopWindow.h"
#include "common/tacMemory.h"

struct TacDesktopApp
{
  TacDesktopApp();
  virtual ~TacDesktopApp() = default;
  virtual void Init( TacErrors& errors ) {}
  virtual void Poll( TacErrors& errors ) {}
  void Loop( TacErrors& errors );
  void SpawnWindowOuter( const TacWindowParams& , TacDesktopWindow** , TacErrors& );
  virtual void GetPrimaryMonitor( TacMonitor* monitor, TacErrors& errors ) = 0;

  virtual void OnShellInit( TacErrors& errors ) {}

  TacVector< TacOwned< TacDesktopWindow > > mMainWindows;

  TacShell* mShell = nullptr;

  static void DoStuff( TacDesktopApp* desktopApp, TacErrors& errors );

protected:
  virtual void SpawnWindow( const TacWindowParams& windowParams, TacDesktopWindow** desktopWindow, TacErrors& errors) {};
};



