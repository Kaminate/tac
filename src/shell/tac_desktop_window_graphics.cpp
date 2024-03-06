#include "src/shell/tac_desktop_window_graphics.h" // self-inc

#include "src/common/graphics/renderer/tac_renderer.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/memory/tac_frame_memory.h"

namespace Tac
{
  static Render::FramebufferHandle sWindowFramebuffers[ kDesktopWindowCapacity ];
  static Render::ViewHandle        sWindowViews[ kDesktopWindowCapacity ];
}

Tac::Render::FramebufferHandle Tac::WindowGraphicsGetFramebuffer( const DesktopWindowHandle& handle )
{
  TAC_ASSERT_INDEX( handle, kDesktopWindowCapacity );
  return sWindowFramebuffers[ ( int )handle ];
}

Tac::Render::ViewHandle Tac::WindowGraphicsGetView( const DesktopWindowHandle& handle )
{
  TAC_ASSERT_INDEX( handle, kDesktopWindowCapacity );
  return sWindowViews[ ( int )handle ];
}

void Tac::WindowGraphicsNativeHandleChanged( const DesktopWindowHandle& desktopWindowHandle,
                                             const void* nativeWindowHandle,
                                             const char* name,
                                             const int x,
                                             const int y,
                                             const int w,
                                             const int h )
{
  TAC_UNUSED_PARAMETER( x );
  TAC_UNUSED_PARAMETER( y );
  if( nativeWindowHandle )
  {
    const int iWindow = desktopWindowHandle.GetIndex();

    const Render::FramebufferHandle framebufferHandle =
      Render::CreateFramebufferForWindow( nativeWindowHandle, w, h, TAC_STACK_FRAME );

    const ShortFixedString dbgname = ShortFixedString::Concat( name, ".", ToString( iWindow ) );

    Render::SetRenderObjectDebugName( framebufferHandle, dbgname );

    const Render::ViewHandle viewHandle = Render::CreateView();

    sWindowFramebuffers[ iWindow ] = framebufferHandle;
    sWindowViews[ iWindow ] = viewHandle;
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect( ( float )w, ( float )h ) );
    Render::SetViewport( viewHandle, Render::Viewport( ( float )w, ( float )h ) );
  }
  else
  {
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( desktopWindowHandle );
    const Render::ViewHandle viewHandle = WindowGraphicsGetView( desktopWindowHandle );
    Render::DestroyFramebuffer( framebufferHandle, TAC_STACK_FRAME );
    Render::DestroyView( viewHandle );
  }
}

void Tac::WindowGraphicsResize( const DesktopWindowHandle& desktopWindowHandle,
                                const int w,
                                const int h )
{
  const Render::FramebufferHandle framebufferHandle = sWindowFramebuffers[ ( int )desktopWindowHandle ];
  const Render::ViewHandle viewHandle = sWindowViews[ ( int )desktopWindowHandle ];
  Render::ResizeFramebuffer( framebufferHandle, w, h, TAC_STACK_FRAME );
  Render::SetViewScissorRect( viewHandle, Render::ScissorRect( ( float )w, ( float )h ) );
  Render::SetViewport( viewHandle, Render::Viewport( ( float )w, ( float )h ) );
}
