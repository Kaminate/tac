#include "tac_window_backend.h" // self-inc
#include "tac_sim_window_api.h"
#include "tac_sys_window_api.h"
#include "tac_app_window_api.h"

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/mutex/tac_mutex.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"

#include "tac-rhi/render3/tac_render_api.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <mutex>
#endif

namespace Tac
{
  struct WindowState
  {
    String                  mName                {};
    v2i                     mPos                 {};
    v2i                     mSize                {};
    bool                    mShown               {};
    Render::SwapChainHandle mSwapChainHandle     {};
    const void*             mNativeWindowHandle  {};
  };

  using WindowStates = Array< WindowState, kDesktopWindowCapacity >;

  static int                       sHandleCounter        {};
  static Vector< int >             sFreeHandles          {};
  static Render::TexFmt            sSwapChainColorFormat { Render::TexFmt::kRGBA16F };
  static Render::TexFmt            sSwapChainDepthFormat { Render::TexFmt::kD24S8 };
  static WindowHandle              sAppHovered           {};
  static WindowStates              sAppCurr              {};
  static bool                      mCreatesSwapChain     { true };

  // -----------------------------------------------------------------------------------------------

  static void         FreeWindowHandle( WindowHandle h )
  {
    sFreeHandles.push_back( h.GetIndex() );
  }

  static WindowHandle AllocWindowHandle()
  {
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

  void AppWindowApiBackend::SetWindowCreated( WindowHandle h,
                                 const void* nwh,
                                 StringView name,
                                 v2i pos,
                                 v2i size,
                                 Errors& errors )
  {
    Render::SwapChainHandle swapChainHandle;
    if( mCreatesSwapChain )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      const Render::SwapChainParams params
      {
        .mNWH      { nwh },
        .mSize     { size },
        .mColorFmt { sSwapChainColorFormat },
        .mDepthFmt { sSwapChainDepthFormat },
      };
      swapChainHandle = renderDevice->CreateSwapChain( params, errors );
    }

    sAppCurr[ h.GetIndex() ] = WindowState
    {
      .mName               { name },
      .mPos                { pos },
      .mSize               { size },
      .mShown              {},
      .mSwapChainHandle    { swapChainHandle },
      .mNativeWindowHandle { nwh },
    };
  }

  void AppWindowApiBackend::SetWindowDestroyed( WindowHandle h )
  {
    WindowState& windowState{ sAppCurr[ h.GetIndex() ] };
    if( mCreatesSwapChain )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      renderDevice->DestroySwapChain( windowState.mSwapChainHandle );
    }

    windowState = {};
  }

  void AppWindowApiBackend::SetWindowIsVisible( WindowHandle h, bool shown )
  {
    sAppCurr[ h.GetIndex() ].mShown = shown;
  }

  void AppWindowApiBackend::SetWindowSize( WindowHandle h, v2i size, Errors& errors )
  {
    WindowState& windowState{ sAppCurr[ h.GetIndex() ] };
    windowState.mSize = size;
    if( mCreatesSwapChain )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      TAC_CALL( renderDevice->ResizeSwapChain( windowState.mSwapChainHandle, size, errors ) );
    }
  }

  void AppWindowApiBackend::SetWindowPos( WindowHandle h, v2i pos )
  {
    sAppCurr[ h.GetIndex() ].mPos = pos;
  }

  void AppWindowApiBackend::SetWindowHovered( WindowHandle h )
  {
    sAppHovered = h;
  }

  v2i  AppWindowApiBackend::GetWindowPos( WindowHandle h )
  {
    return sAppCurr[ h.GetIndex() ].mPos;
  }

  void AppWindowApiBackend::SetCreatesSwapChain( bool createsSwapChain )
  {
    mCreatesSwapChain = createsSwapChain;
  }

  // -----------------------------------------------------------------------------------------------

  bool                    AppWindowApi::IsShown( WindowHandle h)
  {
    return h.IsValid() ? sAppCurr[ h.GetIndex() ].mShown : false;
  }

  bool                    AppWindowApi::IsHovered( WindowHandle h )
  {
    return h.IsValid() ? sAppHovered == h : false;
  }

  v2i                     AppWindowApi::GetPos( WindowHandle h )
  {
    TAC_ASSERT( h.IsValid() );
    return sAppCurr[ h.GetIndex() ].mPos;
  }

  void                    AppWindowApi::SetPos( WindowHandle h, v2i pos)
  {
    TAC_ASSERT( h.IsValid() );
    sAppCurr[ h.GetIndex() ].mPos = pos;

    PlatformFns* platform{ PlatformFns::GetInstance() };
    platform->PlatformSetWindowPos( h, pos );
  }

  v2i                     AppWindowApi::GetSize( WindowHandle h)
  {
    TAC_ASSERT( h.IsValid() );
    return sAppCurr[ h.GetIndex() ].mSize;
  }

  void                    AppWindowApi::SetSize( WindowHandle h, v2i size )
  {
    TAC_ASSERT( h.IsValid() );
    sAppCurr[ h.GetIndex() ].mSize = size;
    
    PlatformFns* platform{ PlatformFns::GetInstance() };
    platform->PlatformSetWindowSize( h, size );
  }

  StringView              AppWindowApi::GetName( WindowHandle h)
  {
    TAC_ASSERT( h.IsValid() );
    return sAppCurr[ h.GetIndex() ].mName;
  }

  const void*             AppWindowApi::GetNativeWindowHandle( WindowHandle h)
  {
    TAC_ASSERT( h.IsValid() );
    return sAppCurr[ h.GetIndex() ].mNativeWindowHandle;
  }

  WindowHandle            AppWindowApi::CreateWindow( WindowCreateParams windowCreateParams,
                                                      Errors& errors )
  {
    const WindowHandle h{ AllocWindowHandle() };
    const PlatformSpawnWindowParams platformParams
    {
      .mHandle { h },
      .mName   { windowCreateParams.mName },
      .mPos    { windowCreateParams.mPos },
      .mSize   { windowCreateParams.mSize },
    };

    PlatformFns* platform { PlatformFns::GetInstance() };
    TAC_CALL_RET( platform->PlatformSpawnWindow( platformParams, errors ) );
    return h;
  }

  void                    AppWindowApi::DestroyWindow( WindowHandle h)
  {
    PlatformFns* platform { PlatformFns::GetInstance() };
    platform->PlatformDespawnWindow( h );
    FreeWindowHandle( h );
  }

  Render::SwapChainHandle AppWindowApi::GetSwapChainHandle( WindowHandle h )
  {
    return sAppCurr[ h.GetIndex() ].mSwapChainHandle;
  }

  void                    AppWindowApi::SetSwapChainAutoCreate( bool autoCreate)
  {
    AppWindowApiBackend::SetCreatesSwapChain( autoCreate );
  }

  void                    AppWindowApi::SetSwapChainColorFormat( Render::TexFmt texFmt )
  {
    sSwapChainColorFormat = texFmt;
  }

  void                    AppWindowApi::SetSwapChainDepthFormat( Render::TexFmt texFmt )
  {
    sSwapChainDepthFormat = texFmt;
  }

  Render::TexFmt          AppWindowApi::GetSwapChainColorFormat() { return sSwapChainColorFormat; }

  Render::TexFmt          AppWindowApi::GetSwapChainDepthFormat() { return sSwapChainDepthFormat; }

  void                    AppWindowApi::DesktopWindowDebugImgui()
  {
#if 0
    if( !ImGuiCollapsingHeader( "DesktopWindowDebugImgui" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    int stateCount {};
    for( int iWindow {}; iWindow < kDesktopWindowCapacity; ++iWindow )
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


} // namespace Tac


