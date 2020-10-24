#include "src/shell/tacDesktopWindowGraphics.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacDesktopWindow.h"

namespace Tac
{
  static Render::FramebufferHandle sWindowFramebuffers[ kDesktopWindowCapacity ];
  static Render::ViewHandle sWindowViews[ kDesktopWindowCapacity ];

  Render::FramebufferHandle WindowGraphicsGetFramebuffer( const DesktopWindowHandle& desktopWindowHandle )
  {
    return sWindowFramebuffers[ desktopWindowHandle.mIndex ];
  }

  Render::ViewHandle WindowGraphicsGetView( const DesktopWindowHandle& desktopWindowHandle )
  {
    return sWindowViews[ desktopWindowHandle.mIndex ];
  }

  void WindowGraphicsNativeHandleChanged( const DesktopWindowHandle& desktopWindowHandle,
                                          void* nativeWindowHandle,
                                          int w,
                                          int h )
  {
    //if( nativeWindowHandle )
    //{
    const Render::FramebufferHandle framebufferHandle = Render::CreateFramebuffer( "", nativeWindowHandle, w, h, TAC_STACK_FRAME );
    const Render::ViewHandle viewHandle = Render::CreateView();
    sWindowFramebuffers[ desktopWindowHandle.mIndex ] = framebufferHandle;
    sWindowViews[ desktopWindowHandle.mIndex ] = viewHandle;
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewScissorRect( viewHandle, ScissorRect( ( float )w, ( float )h ) );
    Render::SetViewport( viewHandle, Viewport( ( float )w, ( float )h ) );
    //}
    //else
    //{
    //  Render::DestroyFramebuffer( sWindowFramebuffers[ desktopWindowHandle.mIndex ], TAC_STACK_FRAME );
    //  sWindowFramebuffers[ desktopWindowHandle.mIndex ] = Render::FramebufferHandle();
    //}
  }

  void WindowGraphicsResize( const DesktopWindowHandle& desktopWindowHandle,
                             int w,
                             int h )
  {
    const Render::FramebufferHandle framebufferHandle = sWindowFramebuffers[ desktopWindowHandle.mIndex ];
    const Render::ViewHandle viewHandle = sWindowViews[ desktopWindowHandle.mIndex ];
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
