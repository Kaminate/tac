#include "tac_window_backend.h" // self-inc

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/platform/tac_platform.h"

import std; // mutex

Tac::WindowBackend::WindowStates Tac::WindowBackend::sGameLogicCurr;

namespace Tac::WindowBackend
{
  using NativeArray = Array< const void*, kWindowCapacity >;

  // need to rename this and describe what this does,
  // since there are now two mutexes in this file
  static std::mutex   sMutex;
  static bool         sModificationAllowed;
  static WindowStates sPlatformCurr;
  static NativeArray  sPlatformNative;

  // Contains data for a window to be created as requested by game logic simulation
  struct SimWindowCreate
  {
    WindowHandle mHandle;
    String       mName; // String (not stringview) for storage
    v2i          mPos;
    v2i          mSize;
  };

  static Vector< SimWindowCreate > sCreateRequests;
  static Vector< WindowHandle >    sDestroyRequests;
  static std::mutex                sRequestMutex;
  static int                       sHandleCounter;
  static Vector< int >             sFreeHandles;
}

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  // Platform thread functions:

  void WindowBackend::ApplyBegin()
  {
    sMutex.lock();
    sModificationAllowed = true;
  }

  void WindowBackend::SetWindowCreated( WindowHandle h,
                                        const void* native,
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
    sPlatformNative[ i ] = native;
  }

  void WindowBackend::SetWindowDestroyed( WindowHandle h )
  {
    TAC_ASSERT( sModificationAllowed );
    const int i = h.GetIndex();
    sPlatformCurr[ i ] = {};
    sPlatformNative[ i ] = {};
  }

  void WindowBackend::SetWindowIsVisible( WindowHandle h, bool shown )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mShown = shown;
  }

  void WindowBackend::SetWindowSize( WindowHandle h, v2i size )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mSize = size;
  }

  void WindowBackend::SetWindowPos( WindowHandle h, v2i pos )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mPos = pos;
  }

  void WindowBackend::ApplyEnd()
  {
    sModificationAllowed = false;
    sMutex.unlock();
  }

  void WindowBackend::PlatformApplyRequests( Errors& errors )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );

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
    sMutex.lock();
    sGameLogicCurr = sPlatformCurr;
    sMutex.unlock();
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

    const CreateRequest request
    {
      .mHandle = WindowHandle{ i },
      .mName = params.mName,
      .mPos = v2i{ params.mX, params.mY },
      .mSize = v2i{ params.mWidth, params.mHeight },
    };
    sCreateRequests.push_back( request );
  }

  void         WindowBackend::GameLogicDestroyWindow( WindowHandle h )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );
    sDestroyRequests.push_back( h );
  }

} // namespace Tac

