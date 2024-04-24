#include "tac_desktop_window_graphics.h" // self-inc

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-engine-core/system/tac_desktop_window.h"
//#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"

namespace Tac
{
  static Render::FramebufferHandle sWindowFramebuffers[ kDesktopWindowCapacity ];
  static Render::ViewHandle        sWindowViews[ kDesktopWindowCapacity ];

  WindowGraphics& WindowGraphics::Instance()
  {
    static WindowGraphics sWindowGraphics;
    return sWindowGraphics;
  }

  Render::FramebufferHandle WindowGraphics::GetFramebuffer( const DesktopWindowHandle& handle )
  {
    const int i = handle.GetIndex();
    TAC_ASSERT_INDEX( i, kDesktopWindowCapacity );
    return sWindowFramebuffers[ i ];
  }

  Render::ViewHandle WindowGraphics::GetView( const DesktopWindowHandle& handle )
  {
    const int i = handle.GetIndex();
    TAC_ASSERT_INDEX( i, kDesktopWindowCapacity );
    return sWindowViews[ i ];
  }


  void WindowGraphics::NativeHandleChanged( const NativeHandleChangedData& data )
  {
    DesktopWindowHandle desktopWindowHandle{ data.mDesktopWindowHandle };
    const void* nativeWindowHandle { data.mNativeWindowHandle };
    const char* name { data.mName };
    const int w { data.mW };
    const int h { data.mH };
    if( nativeWindowHandle )
    {
      const int iWindow { desktopWindowHandle.GetIndex() };

      const Render::FramebufferHandle framebufferHandle{
        Render::CreateFramebufferForWindow( nativeWindowHandle, w, h, TAC_STACK_FRAME ) };

      const ShortFixedString dbgname{ ShortFixedString::Concat( name, ".", ToString( iWindow ) ) };

      Render::SetRenderObjectDebugName( framebufferHandle, dbgname );

      const Render::ViewHandle viewHandle{ Render::CreateView() };

      sWindowFramebuffers[ iWindow ] = framebufferHandle;
      sWindowViews[ iWindow ] = viewHandle;
      Render::SetViewFramebuffer( viewHandle, framebufferHandle );
      Render::SetViewScissorRect( viewHandle, Render::ScissorRect( ( float )w, ( float )h ) );
      Render::SetViewport( viewHandle, Render::Viewport( ( float )w, ( float )h ) );
    }
    else
    {
      const Render::FramebufferHandle framebufferHandle = GetFramebuffer( desktopWindowHandle );
      const Render::ViewHandle viewHandle = GetView( desktopWindowHandle );
      Render::DestroyFramebuffer( framebufferHandle, TAC_STACK_FRAME );
      Render::DestroyView( viewHandle );
    }
  }

  void WindowGraphics::Resize( const DesktopWindowHandle& desktopWindowHandle,
                                  const int w,
                                  const int h )
  {
    const int i{ desktopWindowHandle.GetIndex() };
    const Render::FramebufferHandle framebufferHandle{ sWindowFramebuffers[ i ] };
    const Render::ViewHandle viewHandle{ sWindowViews[ i ] };
    Render::ResizeFramebuffer( framebufferHandle, w, h, TAC_STACK_FRAME );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect( ( float )w, ( float )h ) );
    Render::SetViewport( viewHandle, Render::Viewport( ( float )w, ( float )h ) );
  }
}
