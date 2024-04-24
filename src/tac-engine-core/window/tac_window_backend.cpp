#include "tac_window_backend.h" // self-inc
#include "tac_sim_window_api.h"
#include "tac_sys_window_api.h"

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"


#if TAC_WINDOW_BACKEND_CREATES_SWAP_CHAIN()
#include "tac-rhi/render3/tac_render_api.h"
#endif

import std; // mutex


namespace Tac::WindowBackend
{

  bool SysApi::mIsRendererEnabled = true;
  struct WindowState
  {
    String mName;
    v2i    mPos;
    v2i    mSize;
    bool   mShown;
  };

  using WindowStates = Array< WindowState, kDesktopWindowCapacity >;
  using NWHArray = Array< const void*, kDesktopWindowCapacity >; // Native Window Handles (HWND)
#if TAC_WINDOW_BACKEND_CREATES_SWAP_CHAIN()
  using FBArray = Array< Render::SwapChainHandle, kDesktopWindowCapacity >; // Window Framebuffers
#endif

  // sWindowStateMutex:
  //   Locked by the platform thread inbetween calls to
  //   ApplyBegin/ApplyEnd() to update the sPlatformCurr WindowStates.
  //   (it also updates sPlatformNative and sFramebuffers)
  //
  //   Locked by the simulation thread in GameLogicUpdate()
  //   to copy sGameLogicCurr to sPlatformCurr
  static std::mutex   sWindowStateMutex;
  static WindowStates sGameLogicCurr;
  static WindowStates sPlatformCurr;
  static bool         sModificationAllowed;
  static NWHArray     sPlatformNative;
#if TAC_WINDOW_BACKEND_CREATES_SWAP_CHAIN()
  static FBArray      sFramebuffers;
#endif

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
  //   Locked by the platform thread in SysWindowApi::CreateWindow()
  static std::mutex                sRequestMutex;
  static Vector< SimWindowCreate > sCreateRequests;
  static Vector< WindowHandle >    sDestroyRequests;
  static int                       sHandleCounter;
  static Vector< int >             sFreeHandles;

  // -----------------------------------------------------------------------------------------------

  static void         FreeWindowHandle( WindowHandle h )
  {
    // sRequestMutex must be locked
    sFreeHandles.push_back( h.GetIndex() );
  }

  static WindowHandle AllocWindowHandle()
  {
    // sRequestMutex must be locked

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

    return WindowHandle{ i };
  }

  // -----------------------------------------------------------------------------------------------

  // Platform thread functions:

  void SysApi::ApplyBegin()
  {
    sWindowStateMutex.lock();
    sModificationAllowed = true;
  }

  void SysApi::SetWindowCreated( WindowHandle h,
                                 const void* nwh,
                                 StringView name,
                                 v2i pos,
                                 v2i size,
                                 Errors& errors )
  {
    TAC_ASSERT( sModificationAllowed );
    const int i { h.GetIndex() };
    sPlatformCurr[ i ] = WindowState
    {
      .mName { name },
      .mPos { pos },
      .mSize { size },
      .mShown { false },
    };
    sPlatformNative[ i ] = nwh;

#if TAC_WINDOW_BACKEND_CREATES_SWAP_CHAIN()
    if( mIsRendererEnabled )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      const Render::SwapChainParams params
      {
        .mNWH      { nwh },
        .mSize     { size },
        .mColorFmt { sTexFmt },
      };
      sFramebuffers[ i ] = renderDevice->CreateSwapChain( params, errors );
    }
#endif
  }

  void SysApi::SetWindowDestroyed( WindowHandle h )
  {
    TAC_ASSERT( sModificationAllowed );
    const int i { h.GetIndex() };
    sPlatformCurr[ i ] = {};
    sPlatformNative[ i ] = {};
#if TAC_WINDOW_BACKEND_CREATES_SWAP_CHAIN()
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    if( mIsRendererEnabled )
      renderDevice->DestroySwapChain( sFramebuffers[ i ] );
    sFramebuffers[ i ] = {};
#endif
  }

  void SysApi::SetWindowIsVisible( WindowHandle h, bool shown )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mShown = shown;
  }

  void SysApi::SetWindowSize( WindowHandle h, v2i size )
  {
    TAC_ASSERT( sModificationAllowed );
    const int i { h.GetIndex() };
    sPlatformCurr[ i ].mSize = size;
#if TAC_WINDOW_BACKEND_CREATES_SWAP_CHAIN()
    if( mIsRendererEnabled )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      renderDevice->ResizeSwapChain( sFramebuffers[ i ], size );
    }
#endif
  }

  void SysApi::SetWindowPos( WindowHandle h, v2i pos )
  {
    TAC_ASSERT( sModificationAllowed );
    sPlatformCurr[ h.GetIndex() ].mPos = pos;
  }

  v2i  SysApi::GetWindowPos( WindowHandle h )
  {
    TAC_ASSERT( sModificationAllowed );
    return sPlatformCurr[ h.GetIndex() ].mPos;
  }

  void SysApi::ApplyEnd()
  {
    sModificationAllowed = false;
    sWindowStateMutex.unlock();
  }

  void SysApi::Sync( Errors& errors )
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );

    // We don't need to lock sWindowStateMutex to protect sPlatformNative, 
    // but we do need to lock sWindowStateMutex to protect sPlatformCurr.
    TAC_SCOPE_GUARD( std::lock_guard, sWindowStateMutex );

    PlatformFns* platform { PlatformFns::GetInstance() };

    for( const SimWindowCreate& simParams : sCreateRequests )
    {
      const PlatformSpawnWindowParams platformParams
      {
        .mHandle { simParams.mHandle },
        .mName   { simParams.mName },
        .mPos    { simParams.mPos },
        .mSize   { simParams.mSize },
      };
      TAC_CALL( platform->PlatformSpawnWindow( platformParams, errors ) );
    }

    for( WindowHandle h : sDestroyRequests )
    {
      const int i { h.GetIndex() };
      sPlatformCurr[ i ] = {};
      sPlatformNative[ i ] = {};
      platform->PlatformDespawnWindow( h );
      FreeWindowHandle( h );
    }

    sCreateRequests = {};
    sDestroyRequests = {};
  }

  // -----------------------------------------------------------------------------------------------

  // Sim thread functions:

  void SimApi::Sync()
  {
    sWindowStateMutex.lock();
    sGameLogicCurr = sPlatformCurr;
    sWindowStateMutex.unlock();
  }

  // -----------------------------------------------------------------------------------------------
}

namespace Tac
{
  using namespace WindowBackend;

  WindowHandle SimWindowApi::CreateWindow( WindowCreateParams params ) const
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );

    const WindowHandle h{ AllocWindowHandle() };
    const SimWindowCreate request
    {
      .mHandle { h },
      .mName   { params.mName },
      .mPos    { params.mPos },
      .mSize   { params.mSize },
    };
    sCreateRequests.push_back( request );

    return h;
  }

  void         SimWindowApi::DestroyWindow( WindowHandle h ) const
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );
    sDestroyRequests.push_back( h );
  }

  bool         SimWindowApi::IsShown( WindowHandle h ) const
  {
    return sGameLogicCurr[ h.GetIndex() ].mShown;
  }

  v2i          SimWindowApi::GetPos( WindowHandle h ) const
  {
    return sGameLogicCurr[ h.GetIndex() ].mPos;
  }

  v2i          SimWindowApi::GetSize( WindowHandle h ) const
  {
    return sGameLogicCurr[ h.GetIndex() ].mSize;
  }

  StringView   SimWindowApi::GetName( WindowHandle h ) const
  {
    return sGameLogicCurr[ h.GetIndex() ].mName;
  }

  // -----------------------------------------------------------------------------------------------

  bool             SysWindowApi::IsShown( WindowHandle h ) const
  {
    return sPlatformCurr[ h.GetIndex() ].mShown;
  }

  v2i              SysWindowApi::GetPos( WindowHandle h ) const
  {
    return sPlatformCurr[ h.GetIndex() ].mPos;
  }

  v2i              SysWindowApi::GetSize( WindowHandle h ) const
  {
    return sPlatformCurr[ h.GetIndex() ].mSize;
  }

  StringView       SysWindowApi::GetName( WindowHandle h ) const
  {
    return sPlatformCurr[ h.GetIndex() ].mName;
  }

  const void*      SysWindowApi::GetNWH( WindowHandle h ) const // native window handle
  {
    return sPlatformNative[ h.GetIndex() ];
  }

  WindowHandle     SysWindowApi::CreateWindow( WindowCreateParams params, Errors& errors ) const
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );

    const WindowHandle h{ AllocWindowHandle() };
    const PlatformSpawnWindowParams platformParams
    {
      .mHandle { h },
      .mName   { params.mName },
      .mPos    { params.mPos },
      .mSize   { params.mSize },
    };

    PlatformFns* platform = PlatformFns::GetInstance();
    TAC_CALL_RET( {}, platform->PlatformSpawnWindow( platformParams, errors ) );
    return h;
  }

  void             SysWindowApi::DestroyWindow( WindowHandle h ) const
  {
    TAC_SCOPE_GUARD( std::lock_guard, sRequestMutex );
    PlatformFns* platform = PlatformFns::GetInstance();
    platform->PlatformDespawnWindow( h );
    FreeWindowHandle( h );
  }

#if TAC_WINDOW_BACKEND_CREATES_SWAP_CHAIN()
  Render::SwapChainHandle SysWindowApi::GetSwapChainHandle( WindowHandle h ) const
  {
    return sFramebuffers[ h.GetIndex() ];
  }
#endif

  void             SysWindowApi::DesktopWindowDebugImgui()
  {
#if 0
    if( !ImGuiCollapsingHeader( "DesktopWindowDebugImgui" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    int stateCount { 0 };
    for( int iWindow { 0 }; iWindow < kDesktopWindowCapacity; ++iWindow )
    {
      const DesktopWindowState* state = &sDesktopWindowStates[ iWindow ];
      if( !state->mNativeWindowHandle )
        continue;

      String handleStr = ToString( iWindow );
      while( handleStr.size() < 2 )
        handleStr += ' ';

      String nameStr = state->mName;

      String str;
      str += "Window: " + handleStr + ", ";
      str += "Name: " + nameStr + ", ";
      str += "Pos(" + ToString( state->mX ) + ", " + ToString( state->mY ) + "), ";
      str += "Size( " + ToString( state->mWidth ) + ", " + ToString( state->mHeight ) + "), ";
      str += "Native: " + ToString( ( void* )state->mNativeWindowHandle );

      ImGuiText( str );
      stateCount++;
    }

    if( !stateCount )
      ImGuiText( " No desktop window states (how are you reading this)" );
#endif
  }

}


