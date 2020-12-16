#include "src/shell/tacDesktopWindowGraphics.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacDesktopWindow.h"

namespace Tac
{
  static Render::FramebufferHandle sWindowFramebuffers[ kDesktopWindowCapacity ];
  static Render::ViewHandle sWindowViews[ kDesktopWindowCapacity ];

  Render::FramebufferHandle WindowGraphicsGetFramebuffer( const DesktopWindowHandle& desktopWindowHandle )
  {
    TAC_ASSERT((unsigned)desktopWindowHandle < kDesktopWindowCapacity);
    return sWindowFramebuffers[ (int)desktopWindowHandle ];
  }

  Render::ViewHandle WindowGraphicsGetView( const DesktopWindowHandle& desktopWindowHandle )
  {
    TAC_ASSERT((unsigned)desktopWindowHandle < kDesktopWindowCapacity);
    return sWindowViews[ (int)desktopWindowHandle ];
  }

  void WindowGraphicsNativeHandleChanged( const DesktopWindowHandle& desktopWindowHandle,
                                          const void* nativeWindowHandle,
                                          const int x,
                                          const int y,
                                          const int w,
                                          const int h )
  {
    if( nativeWindowHandle )
    {
      const Render::FramebufferHandle framebufferHandle = Render::CreateFramebuffer( "", nativeWindowHandle, w, h, TAC_STACK_FRAME );
      const Render::ViewHandle viewHandle = Render::CreateView();
      sWindowFramebuffers[ ( int )desktopWindowHandle ] = framebufferHandle;
      sWindowViews[ ( int )desktopWindowHandle ] = viewHandle;
      Render::SetViewFramebuffer( viewHandle, framebufferHandle );
      Render::SetViewScissorRect( viewHandle, ScissorRect( ( float )w, ( float )h ) );
      Render::SetViewport( viewHandle, Viewport( ( float )w, ( float )h ) );
    }
    else
    {
      const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( desktopWindowHandle );
      const Render::ViewHandle viewHandle = WindowGraphicsGetView( desktopWindowHandle );
      Render::DestroyFramebuffer( framebufferHandle, TAC_STACK_FRAME );
      Render::DestroyView( viewHandle );
    }
  }

  void WindowGraphicsResize( const DesktopWindowHandle& desktopWindowHandle,
                             const int w,
                             const int h )
  {
    const Render::FramebufferHandle framebufferHandle = sWindowFramebuffers[ (int)desktopWindowHandle ];
    const Render::ViewHandle viewHandle = sWindowViews[ (int)desktopWindowHandle ];
    Render::ResizeFramebuffer( framebufferHandle, w, h, TAC_STACK_FRAME );
    Render::SetViewScissorRect( viewHandle, ScissorRect( ( float )w, ( float )h ) );
    Render::SetViewport( viewHandle, Viewport( ( float )w, ( float )h ) );
  }

  //void UpdateWindowRenderInterfaces()
  //{
  //  GetDesktopWindowStates();
  //  DesktopWindowStates* oldStates,
  //    DesktopWindowStates* newStates

  //    DesktopWindowStates oldWindowStates = sDesktopWindowStates;
  //  DesktopEventQueue::Instance.ApplyQueuedEvents( &sDesktopWindowStates );
  //  WindowFramebufferManager::Instance.Update( &oldWindowStates,
  //                                             &sDesktopWindowStates );

  //  for( int iDesktopWindowState = 0;
  //       iDesktopWindowState < kDesktopWindowCapacity;
  //       iDesktopWindowState++ )
  //  {
  //    const DesktopWindowState* oldState = &( *oldStates )[ iDesktopWindowState ];
  //    const DesktopWindowState* newState = &( *newStates )[ iDesktopWindowState ];

  //    if( oldState->mNativeWindowHandle &&
  //        newState->mNativeWindowHandle &&
  //        ( oldState->mWidth != newState->mWidth || oldState->mHeight != newState->mHeight ) )
  //    {
  //      WindowFramebufferInfo* info = FindWindowFramebufferInfo( { iDesktopWindowState } );
  //      Render::ResizeFramebuffer( info->mFramebufferHandle,
  //                                 newState->mWidth,
  //                                 newState->mHeight,
  //                                 TAC_STACK_FRAME );
  //    }

  //    if( !oldState->mNativeWindowHandle &&
  //        newState->mNativeWindowHandle )
  //    {
  //      WindowFramebufferInfo info;
  //      info.mDesktopWindowHandle = { iDesktopWindowState }; // newState->mDesktopWindowHandle;
  //      info.mFramebufferHandle = Render::CreateFramebuffer( "",
  //                                                           newState->mNativeWindowHandle,
  //                                                           newState->mWidth,
  //                                                           newState->mHeight,
  //                                                           TAC_STACK_FRAME );
  //      mWindowFramebufferInfos.push_back( info );
  //    }
  //  }
  //}


  //WindowFramebufferInfo* WindowFramebufferManager::FindWindowFramebufferInfo( DesktopWindowHandle desktopWindowHandle )
  //{
  //  for( WindowFramebufferInfo& info : mWindowFramebufferInfos )
  //    if( info.mDesktopWindowHandle.mIndex == desktopWindowHandle.mIndex )
  //      return &info;
  //  return nullptr;
  //}
}
