#pragma once

#include "common/tacUtility.h"
#include "common/tacShell.h"
#include "common/tacErrorHandling.h"
#include "common/tacDesktopWindow.h"
#include "common/tacMemory.h"

struct TacDesktopApp
{
  TacDesktopApp();
  virtual ~TacDesktopApp();
  virtual void Init( TacErrors& errors ) {}
  virtual void Poll( TacErrors& errors ) {}
  void Loop( TacErrors& errors );
  void SpawnWindow( const TacWindowParams& , TacDesktopWindow** , TacErrors& );
  virtual void GetPrimaryMonitor( TacMonitor* monitor, TacErrors& errors ) = 0;

  virtual void OnShellInit( TacErrors& errors ) {}

  TacVector< TacDesktopWindow* > mMainWindows;

  

  static void DoStuff( TacDesktopApp* desktopApp, TacErrors& errors );

protected:
  virtual void SpawnWindowAux( const TacWindowParams& windowParams, TacDesktopWindow** desktopWindow, TacErrors& errors) {};
};



