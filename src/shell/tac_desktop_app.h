#pragma once

#include "src/common/input/tac_keyboard_input.h"
#include "src/common/tac_common.h"

namespace Tac
{
  struct ProjectFns
  {
    void( *mProjectInit )( Errors& ) = []( Errors& ) {};
    void( *mProjectUpdate )( Errors& ) = []( Errors& ) {};
    void( *mProjectUninit )( Errors& ) = []( Errors& ) {};
  };

  struct PlatformFns
  {
    void( *mPlatformFrameBegin )( Errors& ) = nullptr;
    void( *mPlatformFrameEnd )( Errors& ) = nullptr;
    void( *mPlatformSpawnWindow )( const DesktopWindowHandle&,
                                   const char* name,
                                   int x,
                                   int y,
                                   int width,
                                   int height ) = nullptr;
    void( *mPlatformDespawnWindow )( const DesktopWindowHandle& ) = nullptr;
    void( *mPlatformWindowMoveControls )( const DesktopWindowHandle&, const DesktopWindowRect& ) = nullptr;
    void( *mPlatformWindowResizeControls )( const DesktopWindowHandle&, int ) = nullptr;
    DesktopWindowHandle( *mPlatformGetMouseHoveredWindow )( ) = nullptr;
  };

  struct ExecutableStartupInfo
  {
    static ExecutableStartupInfo sInstance;
    String                       mAppName;
    String                       mStudioName = "Sleeping Studio";
    ProjectFns                   mProjectFns;
    static ExecutableStartupInfo Init();
  };


  void                DesktopEventInit();
  void                DesktopEventAssignHandle( const DesktopWindowHandle&,
                                                const void* nativeWindowHandle,
                                                const char* name,
                                                int x,
                                                int y,
                                                int w,
                                                int h );
  void                DesktopEventMoveWindow( const DesktopWindowHandle&, int x, int y );
  void                DesktopEventResizeWindow( const DesktopWindowHandle&, int w, int h );
  void                DesktopEventKeyState( const Keyboard::Key &, bool );
  void                DesktopEventMouseButtonState( const Mouse::Button&, bool );
  void                DesktopEventKeyInput( const Codepoint& );
  void                DesktopEventMouseWheel( int ticks );
  void                DesktopEventMouseMove( const DesktopWindowHandle&, int x, int y );
  void                DesktopEventMouseHoveredWindow( const DesktopWindowHandle& );
  void                DesktopEventApplyQueue();

  void                DesktopAppInit( const PlatformFns&, Errors& );
  void                DesktopAppRun( Errors& );
  DesktopWindowHandle DesktopAppCreateWindow( const char* name = nullptr,
                                              int x = 0,
                                              int y = 0,
                                              int width = 0,
                                              int height = 0 );
  void                DesktopAppDestroyWindow( const DesktopWindowHandle& );
  void                DesktopAppUpdate( Errors& );
  void                DesktopAppResizeControls( const DesktopWindowHandle&, int edgePx = 7 );
  void                DesktopAppMoveControls( const DesktopWindowHandle&, const DesktopWindowRect& );
  void                DesktopAppMoveControls( const DesktopWindowHandle& );
  void                DesktopAppReportErrors();

  Errors&             GetMainErrors();
  bool                IsMainThread();
  bool                IsLogicThread();
} // namespace Tac
