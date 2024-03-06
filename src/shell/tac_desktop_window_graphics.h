#pragma once

namespace Tac
{
  struct DesktopWindowHandle;

  namespace Render
  {
    struct FramebufferHandle;
    struct ViewHandle;
  }

  Render::FramebufferHandle WindowGraphicsGetFramebuffer( const DesktopWindowHandle& );
  Render::ViewHandle        WindowGraphicsGetView( const DesktopWindowHandle& );
  void                      WindowGraphicsNativeHandleChanged( const DesktopWindowHandle&,
                                                               const void* nativeWindowHandle,
                                                               const char* name,
                                                               int x,
                                                               int y,
                                                               int w,
                                                               int h );
  void                      WindowGraphicsResize( const DesktopWindowHandle&,
                                                  int w,
                                                  int h );
}
