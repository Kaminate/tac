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

  void DesktopEventInit();
  void DesktopEventAssignHandle( DesktopWindowHandle,
                                        const void* nativeWindowHandle,
                                        int x,
                                        int y,
                                        int w,
                                        int h );
  void DesktopEventMoveWindow( DesktopWindowHandle,
                                      int x,
                                      int y );
  void DesktopEventResizeWindow( DesktopWindowHandle,
                                        int w,
                                        int h );
  void DesktopEventKeyState( Key, bool );
  void DesktopEventKeyInput( Codepoint );
  void DesktopEventMouseWheel( int ticks );
  void DesktopEventMouseMove( DesktopWindowHandle, int x, int y );
  void DesktopEventMouseHoveredWindow( DesktopWindowHandle );

  void DesktopEventApplyQueue( DesktopWindowState* );

  struct AppInterfaceProject
  {
    void( *mProjectInit )( Errors& ) = 0;
    void( *mProjectUpdate )( Errors& ) = 0;
  };

  struct AppInterfacePlatform
  {
    void( *mPlatformPoll )( Errors& );
    void( *mPlatformSpawnWindow )( const DesktopWindowHandle& handle,
                                   int x,
                                   int y,
                                   int width,
                                   int height );
    DesktopWindowHandle( *mPlatformGetMouseHoveredWindow )( );
  };

  void                    DesktopAppInit( AppInterfacePlatform, Errors& );
  void                    DesktopAppRun( Errors& );
  DesktopWindowHandle     DesktopWindowCreate( int x, int y, int width, int height );
  void                    DesktopWindowPollCreationRequests( Errors& );

  extern Errors           gPlatformThreadErrors;
  extern Errors           gLogicThreadErrors;

  struct WindowHandleIterator
  {
    WindowHandleIterator();
    ~WindowHandleIterator();
    int* begin();
    int* end();
  };

}
