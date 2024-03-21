#include "tac_window_backend.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-rhi/render3/tac_render_api.h"

import std; // mutex

Tac::WindowBackend::WindowStates Tac::WindowBackend::sGameLogicCurr;
Tac::WindowBackend::WindowStates Tac::WindowBackend::sPlatformCurr;

namespace Tac::WindowBackend
{
  using NWHArray = Array< const void*, kDesktopWindowCapacity >; // Native Window Handles (HWND)
  using FBArray = Array< Render::FBHandle, kDesktopWindowCapacity >; // Window Framebuffers

  // sWindowStateMutex:
  //   Locked by the platform thread inbetween calls to
  //   ApplyBegin/ApplyEnd() to update the sPlatformCurr WindowStates.
  //   (it also updates sPlatformNative and sFramebuffers)
  //
  //   Locked by the simulation thread in GameLogicUpdate()
  //   to copy sGameLogicCurr to sPlatformCurr
  static std::mutex   sWindowStateMutex;
  static bool         sModificationAllowed;
  static NWHArray     sPlatformNative;
  static FBArray      sFramebuffers;

  // Contains data for a window to be created as requested by game logic simulation
  struct SimWindowCreate
  {
    WindowHandle mHandle;
    String       mName; // String (not stringview) for storage
    v2i          mPos;
    v2i          mSize;
  };

  // sRequestMutex:
  //   Locked by the simulation thread in GameLogicCreateWindow/GameLogicDestroyWindow()
  //   to update sCreateRequests/sDestroyRequests/sHandleCounter/sFreeHandles
  //
  //   Locked by the platform thread in PlatformApplyRequests() to handle the requests
  static std::mutex                sRequestMutex;
  static Vector< SimWindowCreate > sCreateRequests;
  static Vector< WindowHandle >    sDestroyRequests;
  static int                       sHandleCounter;
  static Vector< int >             sFreeHandles;
}

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  // Platform thread functions:

  void WindowBackend::ApplyBegin()
  {
    sWindowStateMutex.lock();
    sModificationAllowed = true;
  }

  void WindowBackend::SetWindowCreated( WindowHandle h,
                                        const void* nwh,
                                        StringView name,
                                        v2i pos,
                                        v2i size )
  {
    TAC_ASSERT( sModificationAllowed );
    const int i = h.GetIndex();
    sPlatformCurr[ i ] = WindowState
    {
      .mName = name,
      .mPos = pos,
      .mSize = size,
      .mShown = false,
    };
    sPlatformNative[ i ] = nwh;
    sFramebuffers[ i ] = Render::RenderApi::CreateFB( nwh, size );
  }

  void WindowBackend::SetWindowDestroyed( WindowHandle h )
  {
    TAC_ASSERT( sModificationAllowed );
    const int i = h.GetIndex();
    sPlatformCurr[ i ] = {};
    sPlatformNative[ i ] = {};
    Render::RenderApi::DestroyFB( sFramebuffers[ i ] );
    sFramebuffers[ i ] = {};
  }

  void WindowBackend::SetWindowIsVisible( WindowHandle h, bool shown )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mShown = shown;
  }

  void WindowBackend::SetWindowSize( WindowHandle h, v2i size )
  {
    TAC_ASSERT( sModificationAllowed );
    const int i = h.GetIndex();
    sPlatformCurr[ i ].mSize = size;
    Render::RenderApi::ResizeFB( sFramebuffers[ i ], size );
  }

  void WindowBackend::SetWindowPos( WindowHandle h, v2i pos )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mPos = pos;
  }

  void WindowBackend::ApplyEnd()
  {
    sModificationAllowed = false;
    sWindowStateMutex.unlock();
  }

  void WindowBackend::PlatformApplyRequests( Errors& errors )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );

    // We don't need to lock sWindowStateMutex to protect sPlatformNative, 
    // but we do need to lock sWindowStateMutex to protect sPlatformCurr.
    TAC_SCOPE_GUARD( std::lock_guard, sWindowStateMutex );

    PlatformFns* platform = PlatformFns::GetInstance();

    for( const SimWindowCreate& simParams : sCreateRequests )
    {
      const PlatformSpawnWindowParams platformParams
      {
        .mHandle = simParams.mHandle,
        .mName = simParams.mName,
        .mPos = simParams.mPos,
        .mSize = simParams.mSize,
      };
      TAC_CALL( platform->PlatformSpawnWindow( platformParams, errors ) );
    }

    for( WindowHandle h : sDestroyRequests )
    {
      const int i = h.GetIndex();
      sPlatformCurr[ i ] = {};
      sPlatformNative[ i ] = {};
      platform->PlatformDespawnWindow( h );
    }

    sCreateRequests = {};
    sDestroyRequests = {};
  }

  const void* WindowBackend::GetNativeWindowHandle( WindowHandle h )
  {
    return sPlatformNative[ h.GetIndex() ];
  }

  // -----------------------------------------------------------------------------------------------

  // Sim thread functions:

  void         WindowBackend::GameLogicUpdate()
  {
    sWindowStateMutex.lock();
    sGameLogicCurr = sPlatformCurr;
    sWindowStateMutex.unlock();
  }

  WindowHandle WindowBackend::GameLogicCreateWindow( WindowApi::CreateParams params )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );

    int i;
    if( sFreeHandles.empty() )
    {
      i = sHandleCounter++;
    }
    else
    {
      i = sFreeHandles.back();
      sFreeHandles.pop_back();
    }

    const SimWindowCreate request
    {
      .mHandle = WindowHandle{ i },
      .mName = params.mName,
      .mPos = v2i{ params.mX, params.mY },
      .mSize = v2i{ params.mWidth, params.mHeight },
    };
    sCreateRequests.push_back( request );

    return WindowHandle{ i };
  }

  void         WindowBackend::GameLogicDestroyWindow( WindowHandle h )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );
    sDestroyRequests.push_back( h );
  }

} // namespace Tac

