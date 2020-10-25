#pragma once
namespace Tac
{
  namespace Render
  {
    struct FramebufferHandle;
    struct ViewHandle;
  }

  struct DesktopWindowHandle;

  Render::FramebufferHandle WindowGraphicsGetFramebuffer( const DesktopWindowHandle& );
  Render::ViewHandle        WindowGraphicsGetView( const DesktopWindowHandle& );
  void                      WindowGraphicsNativeHandleChanged( const DesktopWindowHandle&,
                                                               const void* nativeWindowHandle,
                                                               int x,
                                                               int y,
                                                               int w,
                                                               int h );
  void                      WindowGraphicsResize( const DesktopWindowHandle&,
                                                  int w,
                                                  int h );
}