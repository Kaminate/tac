#pragma once

#include "src/common/tacUtility.h"
#include "src/common/tacShell.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacMemory.h"
#include "src/common/tackeyboardinput.h"

namespace Tac
{


  enum class ThreadType
  {
    Unknown,
    Main,
    Stuff
  };

  extern thread_local ThreadType gThreadType;

  struct DesktopEventQueue
  {
    static DesktopEventQueue Instance;
    void Init();
    void PushEventCursorUnobscured( DesktopWindowHandle );
    void PushEventCreateWindow( DesktopWindowHandle,
                                int width,
                                int height,
                                int x,
                                int y,
                                void* nativeWindowHandle );
    void PushEventMoveWindow( DesktopWindowHandle,
                              int x,
                              int y );
    void PushEventResizeWindow( DesktopWindowHandle,
                                int w,
                                int h );
    void PushEventKeyState( Key, bool );
    void PushEventKeyInput( Codepoint );
    void PushEventMouseWheel( int ticks );
    void PushEventMouseMove( DesktopWindowHandle, int x, int y );

    void ApplyQueuedEvents( DesktopWindowStateCollection* );
  };

  struct AppInterfaceProject
  {
    void( *mProjectInit )( Errors& ) = 0;
    void( *mProjectUpdate )( Errors& ) = 0;
  };

  struct AppInterfacePlatform
  {
    void( *mPlatformPoll )( Errors& );
    void( *mPlatformSpawnWindow )( DesktopWindowHandle handle,
                                   int x,
                                   int y,
                                   int width,
                                   int height );
  };

  void                    DesktopAppInit( AppInterfacePlatform,
                                          Errors& );
  void                    DesktopAppRun( Errors& );
  extern Errors           sPlatformThreadErrors;
  extern Errors           sLogicThreadErrors;


}
