#pragma once

#include "common/tacUtility.h"
#include "common/tacShell.h"
#include "common/tacErrorHandling.h"
#include "common/tacDesktopWindow.h"
#include "common/tacMemory.h"

struct TacDesktopApp
{
  static TacDesktopApp* Instance;
  TacDesktopApp();
  virtual ~TacDesktopApp();
  virtual void Init( TacErrors& errors );
  virtual void Poll( TacErrors& errors ) {}
  void Run();
  void SpawnWindow( const TacWindowParams& , TacDesktopWindow** , TacErrors& );
  void KillDeadWindows();
  virtual void GetPrimaryMonitor( TacMonitor* monitor, TacErrors& errors ) = 0;
  virtual void SpawnWindowAux( const TacWindowParams& windowParams, TacDesktopWindow** desktopWindow, TacErrors& errors) {};

  TacVector< TacDesktopWindow* > mMainWindows;
  TacErrors mErrorsMainThread;
  TacErrors mErrorsStuffThread;
};
