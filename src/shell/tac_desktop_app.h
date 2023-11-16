#pragma once

#include "src/common/input/tac_keyboard_input.h"
#include "src/common/tac_common.h"
#include "src/common/system/tac_desktop_window.h"

namespace Tac
{
  
  struct ProjectFns
  {
    virtual void ProjectInit( Errors& ) ;
    virtual void ProjectUpdate( Errors& ) ;
    virtual void ProjectUninit( Errors& ) ;
  };

  struct PlatformSpawnWindowParams
  {
    DesktopWindowHandle mHandle;
    const char*         mName;
    int                 mX;
    int                 mY;
    int                 mWidth;
    int                 mHeight;
  };

  struct PlatformFns
  {
    virtual void PlatformImGui( Errors& );
    virtual void PlatformFrameBegin( Errors& );
    virtual void PlatformFrameEnd( Errors& );
    virtual void PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& );
    virtual void PlatformDespawnWindow( const DesktopWindowHandle& );
    virtual void PlatformWindowMoveControls( const DesktopWindowHandle&, const DesktopWindowRect& );
    virtual void PlatformWindowResizeControls( const DesktopWindowHandle&, int );
    virtual DesktopWindowHandle PlatformGetMouseHoveredWindow();
  };

  struct ExecutableStartupInfo
  {
    static ExecutableStartupInfo sInstance;
    String                       mAppName;
    String                       mStudioName = "Sleeping Studio";
    ProjectFns*                  mProjectFns = nullptr;
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

  void                DesktopAppInit( PlatformFns*, Errors& );
  void                DesktopAppRun( Errors& );
  DesktopWindowHandle DesktopAppCreateWindow( const char* name = nullptr,
                                              int x = 0,
                                              int y = 0,
                                              int width = 0,
                                              int height = 0 );
  void                DesktopAppDestroyWindow( const DesktopWindowHandle& );
  void                DesktopAppUpdate( Errors& );
  void                DesktopAppResizeControls( const DesktopWindowHandle&, int edgePx = 7 );
  void                DesktopAppMoveControls( const DesktopWindowHandle&,
                                              const DesktopWindowRect& );
  void                DesktopAppMoveControls( const DesktopWindowHandle& );
  void                DesktopAppDebugImGui(Errors&);
  void                DesktopAppReportErrors();

  Errors&             GetMainErrors();
  bool                IsMainThread();
  bool                IsLogicThread();
} // namespace Tac
