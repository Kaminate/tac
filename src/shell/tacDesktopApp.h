#pragma once

#include "src/common/tackeyboardinput.h"

namespace Tac
{

  struct DesktopWindowHandle;
  struct DesktopWindowRect;
  struct Errors;

  typedef void( *ProjectInit )( Errors& );
  typedef void( *ProjectUpdate )( Errors& );
  typedef void( *ProjectUninit )( Errors& );
  typedef void( *PlatformFrameBegin )( Errors& );
  typedef void( *PlatformFrameEnd )( Errors& );
  typedef void( *PlatformSpawnWindow )( const DesktopWindowHandle&,
                                        int x,
                                        int y,
                                        int width,
                                        int height );
  typedef void( *PlatformDespawnWindow )( const DesktopWindowHandle& );
  typedef void( *PlatformWindowMoveControls )( const DesktopWindowHandle&, const DesktopWindowRect& );
  typedef void( *PlatformWindowResizeControls )( const DesktopWindowHandle&, int );
  typedef DesktopWindowHandle( *PlatformGetMouseHoveredWindow )( );

  struct ExecutableStartupInfo
  {
    void          Init( Errors& );
    String        mAppName;
    String        mStudioName = "Sleeping Studio";
    ProjectInit   mProjectInit = nullptr;
    ProjectUpdate mProjectUpdate = nullptr;
    ProjectUninit mProjectUninit = nullptr;
  };

  struct WindowHandleIterator
  {
    int* begin();
    int* end();
  };

  void                DesktopEventInit();
  void                DesktopEventAssignHandle( DesktopWindowHandle,
                                                const void* nativeWindowHandle,
                                                int x,
                                                int y,
                                                int w,
                                                int h );
  void                DesktopEventMoveWindow( DesktopWindowHandle,
                                              int x,
                                              int y );
  void                DesktopEventResizeWindow( DesktopWindowHandle,
                                                int w,
                                                int h );
  void                DesktopEventKeyState( Key, bool );
  void                DesktopEventKeyInput( Codepoint );
  void                DesktopEventMouseWheel( int ticks );
  void                DesktopEventMouseMove( DesktopWindowHandle, int x, int y );
  void                DesktopEventMouseHoveredWindow( DesktopWindowHandle );
  void                DesktopEventApplyQueue();// DesktopWindowState* );
  void                DesktopAppInit( PlatformSpawnWindow,
                                      PlatformDespawnWindow,
                                      PlatformGetMouseHoveredWindow,
                                      PlatformFrameBegin,
                                      PlatformFrameEnd,
                                      PlatformWindowMoveControls,
                                      PlatformWindowResizeControls,
                                      Errors& );
  void                DesktopAppRun( Errors& );
  DesktopWindowHandle DesktopAppCreateWindow( int x = 0, int y = 0, int width = 0, int height = 0 );
  void                DesktopAppDestroyWindow( DesktopWindowHandle );
  void                DesktopAppUpdate( Errors& );
  void                DesktopAppResizeControls( const DesktopWindowHandle&, int edgePx = 7 );
  void                DesktopAppMoveControls( const DesktopWindowHandle&, DesktopWindowRect windowSpaceRect );
  void                DesktopAppMoveControls( const DesktopWindowHandle& );

  Errors* GetPlatformThreadErrors();
  Errors* GetLogicThreadErrors();
  bool IsMainThread();
  bool IsLogicThread();
}
