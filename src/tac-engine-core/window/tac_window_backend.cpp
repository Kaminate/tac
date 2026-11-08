#include "tac_window_backend.h" // self-inc
#include "tac_app_window_api.h"

#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/mutex/tac_mutex.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"

#include "tac-rhi/render3/tac_render_api.h"

namespace Tac
{

  using WindowStates = Array< AppWindowApiBackend::WindowState, kDesktopWindowCapacity >;

  static WindowHandle::Idx         sHandleCounter        {};
  static Vector< WindowHandle >    sFreedHandles         {};
  static Render::TexFmt            sSwapChainColorFormat { Render::TexFmt::kRGBA16F };
  static Render::TexFmt            sSwapChainDepthFormat { Render::TexFmt::kD24S8 };
  static WindowStates              sAppCurr              {};

  bool          AppWindowApiBackend::sCreatesSwapChain   { true };
  WindowHandle  AppWindowApiBackend::sAppHovered         {};

  auto AppWindowApiBackend::FindWindowHandleFromNWH( NativeWindowHandle nwh ) -> WindowHandle
  {
    for( WindowHandle wh : AppWindowApiBackend::WindowIterator() )
      if( WindowState& state{ sAppCurr[ wh.GetIdx() ] }; state.mNativeWindowHandle == nwh )
        return wh;
    return {};
  }

  auto AppWindowApiBackend::FindWindowStateFromNWH( NativeWindowHandle nwh ) -> WindowState*
  {
    WindowHandle wh{ FindWindowHandleFromNWH( nwh ) };
    return FindWindowStateFromWH( wh );
  }

  //auto AppWindowApiBackend::FindWindowStateFromNWH( NativeWindowHandle nwh ) -> WindowState*
  //{
  //  for( WindowHandle wh : AppWindowApiBackend::WindowIterator() )
  //    if( WindowState& state{ sAppCurr[ wh.GetIdx() ] };
  //        state.mNativeWindowHandle == nwh )
  //      return &state;
  //  return nullptr;
  //}

  auto AppWindowApiBackend::FindWindowStateFromWH( WindowHandle h ) -> WindowState*
  {
    if( WindowHandle::Idx idx{ h.GetIdx() }; idx < kDesktopWindowCapacity )
      if( WindowState* state{ &sAppCurr[ idx ] }; state->mGeneration == h.GetGen() )
        return state;
    return nullptr;
  }

  auto AppWindowApiBackend::WindowIterator::begin() -> WindowIterator
  {
    WindowHandle::Idx idx{};
    while( idx < kDesktopWindowCapacity && !sAppCurr[ idx ].mGeneration )
      idx = ( WindowHandle::Idx )( idx + 1 );

    return WindowIterator{ .mCurrIdx{ idx } };
  }

  auto AppWindowApiBackend::WindowIterator::end()->WindowIterator
  {
    return WindowIterator{ .mCurrIdx{ ( WindowHandle::Idx )kDesktopWindowCapacity} };
  }

  void AppWindowApiBackend::WindowIterator::operator++()
  {
    WindowHandle::Idx idx{ ( WindowHandle::Idx )( mCurrIdx + 1 ) };

    while( idx < kDesktopWindowCapacity && !sAppCurr[ idx ].mGeneration )
      idx = ( WindowHandle::Idx )( idx + 1 );
    mCurrIdx = idx;
  }

  auto AppWindowApiBackend::WindowIterator::operator*() -> WindowHandle
  {
    return WindowHandle( mCurrIdx, sAppCurr[ mCurrIdx ].mGeneration );
  }


  // -----------------------------------------------------------------------------------------------

  bool WindowHandle::IsValid() const
  {
    if( mGen )
      if( AppWindowApiBackend::WindowState* windowState{
          AppWindowApiBackend::FindWindowStateFromWH( *this ) };
          windowState->mGeneration == mGen )
        return true;
    return false;
  }

  // -----------------------------------------------------------------------------------------------

  void AppWindowApiBackend::OnWindowCreated( WindowCreatedParams wcparams, Errors& errors )
  {
    WindowHandle wh{ FindWindowHandleFromNWH( wcparams.mNWH ) };
    WindowState* windowState{ FindWindowStateFromWH( wh ) };
    TAC_ASSERT( windowState );

    Render::SwapChainHandle swapChainHandle{};
    if( sCreatesSwapChain )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      const Render::SwapChainParams scparams
      {
        .mNWH      { wcparams.mNWH.mPointer },
        .mSize     { wcparams.mSize },
        .mColorFmt { sSwapChainColorFormat },
        .mDepthFmt { sSwapChainDepthFormat },
        .mName     { wcparams.mName },
      };
      swapChainHandle = renderDevice->CreateSwapChain( scparams, errors );
    }

    windowState->mName = wcparams.mName;
    windowState->mPos = wcparams.mPos;
    windowState->mSize = wcparams.mSize;
    windowState->mShown = false;
    windowState->mSwapChainHandle = swapChainHandle;
  }

  // The aftermath of WM_DESTROY
  void AppWindowApiBackend::OnWindowDestroyed( NativeWindowHandle h )
  {
    WindowHandle wh{ FindWindowHandleFromNWH( h ) };
    if( WindowState * windowState{ FindWindowStateFromWH( wh ) } )
    {
      if( sCreatesSwapChain )
      {
        Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
        renderDevice->DestroySwapChain( windowState->mSwapChainHandle );
      }

      ImGuiSaveWindowSettings( wh );

      sFreedHandles.push_back( wh );
      *windowState = {};
    }
  }

  void AppWindowApiBackend::OnWindowSize( NativeWindowHandle h, v2i size, Errors& errors )
  {
    if( WindowState * windowState{ FindWindowStateFromNWH( h ) } )
    {
      windowState->mSize = size;
      windowState->mShown = ( size.x > 0 ) && ( size.y > 0 );
      if( sCreatesSwapChain )
      {
        Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
        TAC_CALL( renderDevice->ResizeSwapChain( windowState->mSwapChainHandle, size, errors ) );
      }
    }
  }

  //void AppWindowApiBackend::SetWindowPos( WindowHandle h, v2i pos )
  //{
  //  if( WindowState * windowState{ FindWindowStateFromWH( h ) } )
  //    windowState->mPos = pos;
  //}

  //void AppWindowApiBackend::SetWindowHovered( WindowHandle h )
  //{
  //  sAppHovered = h;
  //}

  //auto AppWindowApiBackend::GetWindowPos( WindowHandle h ) -> v2i
  //{
  //  if( WindowState * windowState{ FindWindowStateFromWH( h ) } )
  //    return windowState->mPos;
  //  TAC_ASSERT( false ); // ???
  //  return {};
  //}

  //auto AppWindowApiBackend::GetWindowNWH( WindowHandle h ) -> NativeWindowHandle
  //{
  //  if( WindowState * windowState{ FindWindowStateFromWH( h ) } )
  //    return windowState->mNativeWindowHandle;

  //  TAC_ASSERT( false ); // ???
  //  return {};
  //}

  //void AppWindowApiBackend::SetCreatesSwapChain( bool createsSwapChain )
  //{
  //  mCreatesSwapChain = createsSwapChain;
  //}

  // -----------------------------------------------------------------------------------------------

  bool AppWindowApi::IsShown( WindowHandle h)
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
      return windowState->mShown;
    return {};
  }

  bool AppWindowApi::IsHovered( WindowHandle h )
  {
    return h == AppWindowApiBackend::sAppHovered;
  }

  auto AppWindowApi::GetPos( WindowHandle h ) -> v2i
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
      return windowState->mPos;
    TAC_ASSERT( false ); //???
    return {};
  }

  void AppWindowApi::SetPos( WindowHandle h, v2i pos)
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
    {
      windowState->mPos = pos;
      Platform::PlatformSetWindowPos( windowState->mNativeWindowHandle, pos );
    }
  }

  auto AppWindowApi::GetSize( WindowHandle h) -> v2i
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
      return windowState->mSize;

    TAC_ASSERT( false ); //???
    return {};
  }

  void AppWindowApi::SetSize( WindowHandle h, v2i size )
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
    {
      windowState->mSize = size;
      Platform::PlatformSetWindowSize( windowState->mNativeWindowHandle, size );
    }
  }

  auto AppWindowApi::GetName( WindowHandle h ) -> StringView
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
      return windowState->mName;

    TAC_ASSERT( false ); //???
    return {};
  }

  auto AppWindowApi::GetNativeWindowHandle( WindowHandle h) -> NativeWindowHandle
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
      return windowState->mNativeWindowHandle;
    TAC_ASSERT( false ); //???
    return {};
  }

  auto AppWindowApi::CreateWindow( WindowCreateParams params, Errors& errors ) -> WindowHandle
  {
    WindowHandle::Idx idx{};
    WindowHandle::Gen gen{};
    if( sFreedHandles.empty() )
    {
      idx = sHandleCounter;
      gen = ( WindowHandle::Gen )1;
      sHandleCounter = ( WindowHandle::Idx )( sHandleCounter + 1 );
    }
    else
    {
      const WindowHandle freedHandle{ sFreedHandles.back() };
      sFreedHandles.pop_back();
      idx = freedHandle.GetIdx();
      gen = ( WindowHandle::Gen )( freedHandle.GetGen() + 1 );
    }

    const WindowHandle h( idx, gen );

    NativeWindowHandle nwh{};
    TAC_CALL_RET( nwh = Platform::PlatformSpawnWindow(
      PlatformSpawnWindowParams
      {
        .mHandle { h },
        .mName   { params.mName },
        .mPos    { params.mPos },
        .mSize   { params.mSize },
      }, errors ) );

    sAppCurr[ idx ] = AppWindowApiBackend::WindowState
    {
      .mGeneration = gen,
      .mName = params.mName,
      .mPos = params.mPos,
      .mSize = params.mSize,
      .mShown = false,
      .mSwapChainHandle = {},
      .mNativeWindowHandle = nwh,
    };

    return h;
  }

  // The application requests for this window to be destroyed.
  // It will not actually be destroyed until after WM_DESTROY,
  // after which AppWindowApiBackend::SetWindowDestroyed takes care of the rest
  void AppWindowApi::DestroyWindow( WindowHandle h )
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
      Platform::PlatformDespawnWindow( windowState->mNativeWindowHandle );
  }

  void AppWindowApi::MinimizeWindow( WindowHandle h )
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
    {
      windowState->mShown = false;
      Platform::PlatformMinimizeWindow( windowState->mNativeWindowHandle );
    }
  }

  auto AppWindowApi::GetSwapChainHandle( WindowHandle h ) -> Render::SwapChainHandle
  {
    if( AppWindowApiBackend::WindowState * windowState{
      AppWindowApiBackend::FindWindowStateFromWH( h ) } )
      return windowState->mSwapChainHandle;
    TAC_ASSERT( false ); //???
    return {};
  }

  void AppWindowApi::SetSwapChainAutoCreate( bool autoCreate)
  {
    AppWindowApiBackend::sCreatesSwapChain = autoCreate;
  }

  void AppWindowApi::SetSwapChainColorFormat( Render::TexFmt texFmt )
  {
    sSwapChainColorFormat = texFmt;
  }

  void AppWindowApi::SetSwapChainDepthFormat( Render::TexFmt texFmt )
  {
    sSwapChainDepthFormat = texFmt;
  }

  auto AppWindowApi::GetSwapChainColorFormat() -> Render::TexFmt { return sSwapChainColorFormat; }

  auto AppWindowApi::GetSwapChainDepthFormat() -> Render::TexFmt { return sSwapChainDepthFormat; }

  void AppWindowApi::DesktopWindowDebugImgui()
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


