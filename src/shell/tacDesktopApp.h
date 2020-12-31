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

  typedef void( *ProjectInit )( Errors& );
  typedef void( *ProjectUpdate )( Errors& );
  typedef void( *ProjectUninit )( Errors& );
  //typedef void( *PlatformPoll )( Errors& );
  typedef void( *PlatformFrameBegin )( Errors& );
  typedef void( *PlatformFrameEnd )( Errors& );
  typedef void( *PlatformSpawnWindow )( const DesktopWindowHandle& handle,
                                        int x,
                                        int y,
                                        int width,
                                        int height );
  typedef DesktopWindowHandle( *PlatformGetMouseHoveredWindow )( );
  typedef void( *PlatformWindowMoveControls )( const DesktopWindowHandle&, const DesktopWindowRect& );
  typedef void( *PlatformWindowResizeControls )( const DesktopWindowHandle&, int );

  struct ExecutableStartupInfo
  {
    void          Init( Errors& );
    String        mAppName;
    String        mStudioName = "Sleeping Studio";
    ProjectInit   mProjectInit;
    ProjectUpdate mProjectUpdate;
    ProjectUninit mProjectUninit;
  };

  struct WindowHandleIterator
  {
    WindowHandleIterator();
    ~WindowHandleIterator();
    int* begin();
    int* end();
  };

  void                           DesktopEventInit();
  void                           DesktopEventAssignHandle( DesktopWindowHandle,
                                                           const void* nativeWindowHandle,
                                                           int x,
                                                           int y,
                                                           int w,
                                                           int h );
  void                           DesktopEventMoveWindow( DesktopWindowHandle,
                                                         int x,
                                                         int y );
  void                           DesktopEventResizeWindow( DesktopWindowHandle,
                                                           int w,
                                                           int h );
  void                           DesktopEventKeyState( Key, bool );
  void                           DesktopEventKeyInput( Codepoint );
  void                           DesktopEventMouseWheel( int ticks );
  void                           DesktopEventMouseMove( DesktopWindowHandle, int x, int y );
  void                           DesktopEventMouseHoveredWindow( DesktopWindowHandle );
  void                           DesktopEventApplyQueue();// DesktopWindowState* );
  void                           DesktopAppInit( PlatformSpawnWindow,
                                                 PlatformGetMouseHoveredWindow,
                                                 PlatformFrameBegin,
                                                 PlatformFrameEnd,
                                                 PlatformWindowMoveControls,
                                                 PlatformWindowResizeControls,
                                                 Errors& );
  void                           DesktopAppRun( Errors& );
  DesktopWindowHandle            DesktopAppCreateWindow( int x = 0, int y = 0, int width = 0, int height = 0 );
  void                           DesktopAppUpdate( Errors& );
  void                           DesktopAppResizeControls( const DesktopWindowHandle&, int edgePx = 7 );
  void                           DesktopAppMoveControls( const DesktopWindowHandle&, DesktopWindowRect windowSpaceRect );
  void                           DesktopAppMoveControls( const DesktopWindowHandle& );
  extern Errors                  gPlatformThreadErrors;
  extern Errors                  gLogicThreadErrors;
  extern thread_local ThreadType gThreadType;
}
