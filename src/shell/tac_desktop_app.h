#pragma once

#include "src/common/input/tac_keyboard_input.h"
#include "src/common/tac_common.h"

namespace Tac
{


  typedef void( *ProjectInit )( Errors& );
  typedef void( *ProjectUpdate )( Errors& );
  typedef void( *ProjectUninit )( Errors& );
  typedef void( *PlatformFrameBegin )( Errors& );
  typedef void( *PlatformFrameEnd )( Errors& );
  typedef void( *PlatformSpawnWindow )( const DesktopWindowHandle&,
                                        const char* name,
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
    static ExecutableStartupInfo sInstance;
    String        mAppName;
    String        mStudioName = "Sleeping Studio";
    ProjectInit   mProjectInit = nullptr;
    ProjectUpdate mProjectUpdate = nullptr;
    ProjectUninit mProjectUninit = nullptr;
    static ExecutableStartupInfo Init();
  };

  //ExecutableStartupInfo InitExecutableStartupInfo();
  //struct WindowHandleIterator
  //{
  //  int* begin();
  //  int* end();
  //};

  void                DesktopEventInit();
  void                DesktopEventAssignHandle( DesktopWindowHandle,
                                                const void* nativeWindowHandle,
                                                const char* name,
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
  void                DesktopEventKeyState( Keyboard::Key, bool );
  void                DesktopEventMouseButtonState( Mouse::Button, bool );
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
  DesktopWindowHandle DesktopAppCreateWindow( const char* name = nullptr,
                                              int x = 0,
                                              int y = 0,
                                              int width = 0,
                                              int height = 0 );
  void                DesktopAppDestroyWindow( DesktopWindowHandle );
  void                DesktopAppUpdate( Errors& );
  void                DesktopAppResizeControls( const DesktopWindowHandle&, int edgePx = 7 );
  void                DesktopAppMoveControls( const DesktopWindowHandle&, DesktopWindowRect windowSpaceRect );
  void                DesktopAppMoveControls( const DesktopWindowHandle& );
  void                DesktopAppReportErrors();

  Errors&             GetMainErrors();
  bool                IsMainThread();
  bool                IsLogicThread();
}
