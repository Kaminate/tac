#pragma once

#include "src/common/input/tac_keyboard_input.h"
#include "src/common/tac_core.h"
#include "src/common/system/tac_desktop_window.h"

namespace Tac
{

  struct App
  {
    struct Config
    {
      String mName;
      String mStudioName = "Sleeping Studio";

      //     Can disable the renderer for headless apps or for apps that define their own renderer
      bool   mDisableRenderer = false;
    };

    App(const Config& );
    virtual ~App() {};

    virtual void Init( Errors& ) {};
    virtual void Update( Errors& ) {};
    virtual void Uninit( Errors& ) {};

    static App*  Create();

    Config mConfig;
  };

  // -----------------------------------------------------------------------------------------------

  struct DesktopAppCreateWindowParams
  {
    const char* mName = "";
    int         mX = 0;
    int         mY = 0;
    int         mWidth = 0;
    int         mHeight = 0;
  };

  // -----------------------------------------------------------------------------------------------

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

  // -----------------------------------------------------------------------------------------------

  //void                DesktopEventAssignHandle( const DesktopWindowHandle&,
  //                                              const void* nativeWindowHandle,
  //                                              const char* name,
  //                                              int x,
  //                                              int y,
  //                                              int w,
  //                                              int h );
  //void                DesktopEventMoveWindow( const DesktopWindowHandle&, int x, int y );
  //void                DesktopEventResizeWindow( const DesktopWindowHandle&, int w, int h );
  //void                DesktopEventKeyState( const Keyboard::Key &, bool );
  //void                DesktopEventMouseButtonState( const Mouse::Button&, bool );
  //void                DesktopEventKeyInput( const Codepoint& );
  //void                DesktopEventMouseWheel( int ticks );
  //void                DesktopEventMouseMove( const DesktopWindowHandle&, int x, int y );
  //void                DesktopEventMouseHoveredWindow( const DesktopWindowHandle& );

  // -----------------------------------------------------------------------------------------------

  void                DesktopAppInit( PlatformFns*, Errors& );
  void                DesktopAppRun( Errors& );
  DesktopWindowHandle DesktopAppCreateWindow( const DesktopAppCreateWindowParams& );
  void                DesktopAppDestroyWindow( const DesktopWindowHandle& );
  void                DesktopAppUpdate( Errors& );
  void                DesktopAppResizeControls( const DesktopWindowHandle&, int edgePx = 7 );
  void                DesktopAppMoveControls( const DesktopWindowHandle&,
                                              const DesktopWindowRect& );
  void                DesktopAppMoveControls( const DesktopWindowHandle& );
  void                DesktopAppDebugImGui(Errors&);

  // -----------------------------------------------------------------------------------------------

  Errors&             GetMainErrors();
  bool                IsMainThread();
  bool                IsLogicThread();
} // namespace Tac
