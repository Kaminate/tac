#pragma once

#include "src/common/tacUtility.h"
#include "src/common/tacShell.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacMemory.h"
#include <thread>

namespace Tac
{


enum class ThreadType
{
  Unknown,
  Main,
  Stuff
};

extern thread_local ThreadType gThreadType;

struct DesktopApp
{
  static DesktopApp* Instance;
  DesktopApp();
  virtual ~DesktopApp();
  virtual void Init( Errors& errors );
  virtual void Poll( Errors& errors ) {}
  void Run();
  void SpawnWindow( const WindowParams&, DesktopWindow**, Errors& );
  void KillDeadWindows();
  virtual void GetPrimaryMonitor( Monitor* monitor, Errors& errors ) = 0;
  virtual void SpawnWindowAux( const WindowParams& windowParams, DesktopWindow** desktopWindow, Errors& errors ) {};
  DesktopWindow* FindWindow( StringView windowName );

  Vector< DesktopWindow* > mMainWindows;
  Errors mErrorsMainThread;
  Errors mErrorsStuffThread;
  std::thread mStuffThread;
};
}
