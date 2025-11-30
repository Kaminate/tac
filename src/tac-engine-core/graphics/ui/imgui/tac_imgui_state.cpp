#include "tac_imgui_state.h" // self-inc

#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/tac_text_edit.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-rhi/render3/tac_render_api.h" // CreateContext
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

#define TAC_IMGUI_RESIZE_DEBUG() TAC_IS_DEBUG_MODE() && false


namespace Tac
{
  struct RegisteredWindowResource
  {
    String         mName        {};
    Vector< char > mInitialData {};
    ImGuiRscIdx    mId          {};
  };

  struct WindowResourceRegistry
  {
    static auto GetInstance() -> WindowResourceRegistry*;
    auto RegisterResource( ImGuiWindowResource::Params ) -> ImGuiRscIdx;
    auto FindResource( ImGuiRscIdx index ) -> RegisteredWindowResource*;
  private:
    Vector< RegisteredWindowResource > mRegisteredWindowResources;
    int                                mResourceCounter {};
  };

  // -----------------------------------------------------------------------------------------------

  ImGuiGlobals                ImGuiGlobals::Instance;

  // -----------------------------------------------------------------------------------------------

  auto WindowResourceRegistry::FindResource( ImGuiRscIdx index ) -> RegisteredWindowResource*
  {
    for( RegisteredWindowResource& resource : mRegisteredWindowResources )
      if( resource.mId == index )
        return &resource;
    return nullptr;
  }

  auto WindowResourceRegistry::GetInstance() -> WindowResourceRegistry*
  {
    static WindowResourceRegistry instance;
    return &instance;
  }

  auto WindowResourceRegistry::RegisterResource(ImGuiWindowResource::Params params) -> ImGuiRscIdx
  {
    Vector< char > initialData;
    if( params.mInitialDataBytes )
    {
      const char* dataBegin { ( char* )params.mInitialDataBytes };
      const char* dataEnd { ( char* )params.mInitialDataBytes + params.mInitialDataByteCount };
      initialData.assign( dataBegin, dataEnd );
    }
    else
    {
      initialData.assign( params.mInitialDataByteCount, 0 );
    }
    const ImGuiRscIdx id { mResourceCounter++ };
    mRegisteredWindowResources.push_back(
      RegisteredWindowResource
      {
        .mName        { params.mName },
        .mInitialData { initialData },
        .mId          { id },
      } );
    return id;
  }

  // -----------------------------------------------------------------------------------------------

  void ImGuiWindow::Scrollbar()
  {
    ImGuiGlobals& globals { ImGuiGlobals::Instance };

    const bool scrollBarEnabled { globals.mScrollBarEnabled };
    if( !scrollBarEnabled )
      return;

    const bool stuffBelowScreen { mViewportSpaceMaxiCursor.y > mViewportSpaceVisibleRegion.mMaxi.y };
    const bool stuffAboveScreen{ ( bool )mScroll };
    if( !stuffBelowScreen && !stuffAboveScreen )
      return;

    mDrawData->PushDebugGroup( "scrollbar" );
    TAC_ON_DESTRUCT( mDrawData->PopDebugGroup() );

    const float scrollbarWidth { 30 };


    const ImGuiID id{ GetID( "##SCROLLBAR" ) };
    const v2 scrollbarBackgroundMini { mViewportSpacePos + v2( mSize.x - scrollbarWidth, 0 ) };
    const v2 scrollbarBackgroundMaxi { mViewportSpacePos + mSize };

    const UI2DDrawData::Box bg
    {
      .mMini  { scrollbarBackgroundMini },
      .mMaxi  { scrollbarBackgroundMaxi },
      .mColor { ImGuiGetColor( ImGuiCol::ScrollbarBG ) },
    };

    mDrawData->AddBox( bg );

    float contentAllMinY { mViewportSpacePos.y - mScroll };
    float contentAllMaxY { mViewportSpaceMaxiCursor.y };
    float contentAllHeight { contentAllMaxY - contentAllMinY };
    float contentVisibleMinY { mViewportSpacePos.y };
    float contentVisibleMaxY { mViewportSpacePos.y + mSize.y };
    float contentVisibleHeight { contentVisibleMaxY - contentVisibleMinY };

    // scrollbar min/max position
    const float scrollMin {};
    const float scrollMax { contentAllHeight - contentVisibleHeight };

    // scroll with middle mouse
    if( !GetActiveID().IsValid()
        && IsHovered( ImGuiRect::FromPosSize( mViewportSpacePos, mSize ), id )
        && UIKeyboardApi::GetMouseWheelDelta() )
      mScroll = Clamp( mScroll - UIKeyboardApi::GetMouseWheelDelta() * 40.0f, scrollMin, scrollMax );

    const float scrollbarForegroundMiniX{ 3 + scrollbarBackgroundMini.x };
    const float scrollbarForegroundMiniY{ 3
      + mViewportSpacePos.y
      + ( ( contentVisibleMinY - contentAllMinY ) / contentAllHeight ) * mSize.y, };

    const v2 scrollbarForegroundMini( scrollbarForegroundMiniX,
                                      scrollbarForegroundMiniY );

    const float scrollbarForegroundMaxiX{ -3 + scrollbarBackgroundMaxi.x };
    const float scrollbarForegroundMaxiY{ -3
      + mViewportSpacePos.y
      + ( ( contentVisibleMaxY - contentAllMinY ) / contentAllHeight ) * mSize.y,
    };
    const v2 scrollbarForegroundMaxi( scrollbarForegroundMaxiX,
                                      scrollbarForegroundMaxiY );

    const ImGuiRect scrollbarForegroundRect{ ImGuiRect::FromMinMax( scrollbarForegroundMini,
                                                                     scrollbarForegroundMaxi ) };

    const bool hovered { IsHovered( scrollbarForegroundRect, id ) };
    if( hovered )
    {
      if( UIKeyboardApi::JustPressed( Key::MouseLeft ) )
      {
        SetActiveID( id, this );
        mScrollMousePosScreenspaceInitial = UIKeyboardApi::GetMousePosScreenspace();
      }
    }

    const float scrollbarHeight { scrollbarForegroundRect.GetHeight() };
    const bool active { GetActiveID() == id };

    v4 barColor{ ImGuiGetColor( ImGuiCol::Scrollbar ) };
    if( hovered )
      barColor =  ImGuiGetColor( ImGuiCol::ScrollbarHovered );
    if( active )
      barColor =  ImGuiGetColor( ImGuiCol::ScrollbarActive );

    mDrawData->AddBox( 
      UI2DDrawData::Box
      {
        .mMini  { scrollbarForegroundMini },
        .mMaxi  { scrollbarForegroundMaxi },
        .mColor { barColor },
      } );


    if(active)
    {
      const float mouseDY{
        UIKeyboardApi::GetMousePosScreenspace().y
       - mScrollMousePosScreenspaceInitial.y };
      mScrollMousePosScreenspaceInitial.y = (float)UIKeyboardApi::GetMousePosScreenspace().y;
      const float scrollDY{ mouseDY * ( contentVisibleHeight / scrollbarHeight ) };
      mScroll = Clamp( mScroll + scrollDY , scrollMin, scrollMax );


      if( !UIKeyboardApi::IsPressed( Key::MouseLeft ) )
      {
        ClearActiveID();
      }
    }

    mViewportSpaceVisibleRegion.mMaxi.x -= scrollbarWidth;
  }

  void ImGuiWindow::DrawWindowBackground()
  {
    if( const bool drawWindow{ mEnableBG
        && ( mParent || mStretchWindow || mWindowHandleOwned ) } )
    {
      const ImGuiRect origRect { ImGuiRect::FromPosSize( mViewportSpacePos, mSize ) };
      if( Overlaps( origRect ) )
      {
        const ImGuiRect clipRect { Clip( origRect ) };
        const ImGuiCol col{
          mParent ? ImGuiCol::ChildWindowBackground : ImGuiCol::WindowBackground };
        const UI2DDrawData::Box box
        {
          .mMini  { clipRect.mMini },
          .mMaxi  { clipRect.mMaxi },
          .mColor { ImGuiGetColor( col ) },
        };
        mDrawData->AddBox( box, &clipRect );
      }
    }
  }

  void ImGuiWindow::EndFrame()
  {
    mDrawData->PopDebugGroup(); // pushed in ImGuiWindow::BeginFrame
  }

  void ImGuiWindow::BeginFrame()
  {
    const UIStyle& style{ ImGuiGetStyle() };
    const float windowPaddingPx{ style.windowPadding
      * mDesktopWindow->mMonitorDpi
      / ImGuiGlobals::Instance.mReferenceResolution.mDpi
    };

    if( !mParent )
      mDrawData->clear();

    // popped in ImGuiWindow::EndFrame
    mDrawData->PushDebugGroup( ShortFixedString::Concat( "BeginFrame(" , mName , ")" ) );

    mIDStack.clear();
    mIDStack.push_back( mWindowID = Hash( mName ) );
    mMoveID = GetID( "#MOVE" );

    mViewportSpacePos = mParent ? mParent->mViewportSpaceCurrCursor : mViewportSpacePos;
    DrawWindowBackground();
    mViewportSpaceVisibleRegion = ImGuiRect::FromPosSize( mViewportSpacePos, mSize );
    Scrollbar();
    mViewportSpaceVisibleRegion.mMini += v2( 1, 1 ) * windowPaddingPx;
    mViewportSpaceVisibleRegion.mMaxi -= v2( 1, 1 ) * windowPaddingPx;
    const v2 drawPos { mViewportSpaceVisibleRegion.mMini - v2( 0, mScroll ) };
    mViewportSpaceCurrCursor = drawPos;
    mViewportSpacePrevCursor = drawPos;
    mViewportSpaceMaxiCursor = drawPos;
    mCurrLineHeight = 0;
    mPrevLineHeight = 0;

    mXOffsets.clear();
    PushXOffset();

    if( mParent )
    {
      mDesktopWindow = mParent->mDesktopWindow;
      mWindowHandleOwned = false;
    }

    ResizeControls();
  }

  void ImGuiWindow::UpdateMoveControls()
  {
    dynmc ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    if( UIKeyboardApi::IsPressed( Key::MouseLeft ) )
    {
      mDesktopWindow->mRequestedPosition = GetMousePosViewport()
                                         - globals.mActiveIDClickPos_VS
                                         + mViewportSpacePos
                                         + GetWindowPosScreenspace();


      globals.mSettingsDirty = true;
    }
    else
    {
      globals.mMovingWindow = nullptr;
      ClearActiveID();
    }
  }

  void ImGuiWindow::BeginMoveControls()
  {
    dynmc ImGuiGlobals& globals{ ImGuiGlobals::Instance };

    if( !UIKeyboardApi::JustPressed( Key::MouseLeft ) )
      return;

    if( !IsHovered( ImGuiRect::FromPosSize( mViewportSpacePos, mSize ), mMoveID ) )
      return;
      
    if( globals.mActiveID.IsValid() || globals.mHoveredID.IsValid() )
      return;
    
    if( mDesktopWindow->mWindowHandle != globals.mMouseHoveredWindow )
      return;

    SetActiveID( mMoveID, this );
    globals.mActiveIDClickPos_VS = GetMousePosViewport();
    globals.mActiveIDWindowPos_SS = GetWindowPosScreenspace();
    globals.mMovingWindow = this;
  }

  void ImGuiWindow::ResizeControls()
  {
    if( mParent )
      return;

    dynmc ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    const ImGuiID id{ GetID( "##RESIZE" ) };
    const UIStyle style{ globals.mUIStyle };
    const float windowPaddingPx{ style.windowPadding
      * mDesktopWindow->mMonitorDpi
      / globals.mReferenceResolution.mDpi 
    };
    const v2i viewportPos_SS{ AppWindowApi::GetPos( mDesktopWindow->mWindowHandle ) };
    const v2 mousePos_VS{ GetMousePosViewport() };
    const ImGuiRect origWindowRect_VS{ ImGuiRect::FromPosSize( mViewportSpacePos, mSize ) };
    dynmc ImGuiRect targetWindowRect_VS{ origWindowRect_VS };
    const int E { 1 << 0 };
    const int S { 1 << 1 };
    const int W { 1 << 2 };
    const int N { 1 << 3 };
    dynmc int hoverMask {};
    const bool anyEdgeActive{ globals.mActiveID == id };

    for( int iSide{}; iSide < 4; ++iSide )
    {
        ImGuiRect edgeRect_VS{ origWindowRect_VS };

        const int dir{ 1 << iSide };
        switch( dir )
        {
        case E: edgeRect_VS.mMini.x = edgeRect_VS.mMaxi.x - windowPaddingPx; break;
        case S: edgeRect_VS.mMini.y = edgeRect_VS.mMaxi.y - windowPaddingPx; break;
        case W: edgeRect_VS.mMaxi.x = edgeRect_VS.mMini.x + windowPaddingPx; break;
        case N: edgeRect_VS.mMaxi.y = edgeRect_VS.mMini.y + windowPaddingPx; break;
        }

        const bool hovered{ IsHovered( edgeRect_VS, id ) };
        const bool edgeActive{ anyEdgeActive && ( globals.mResizeMask & dir ) };
        dynmc float& alphaCur{ mBorderData[ iSide ].x };
        dynmc float& alphaVel{ mBorderData[ iSide ].y };
        dynmc float alphaTgt = 0.05f;
        if( edgeActive ) { alphaTgt += .6f; }
        else if( hovered ) {
          const GameTimeDelta hoverTime{
            ImGuiGlobals::Instance.mElapsedSeconds -
            ImGuiGlobals::Instance.mHoverStartTime };
          alphaTgt += 0.2f * ( .5f + .5f * ( float )Sin( -3.14f / 2 + hoverTime.mSeconds * 4 ) );
        }
        
        const float kAlphaEps{ 0.01f };
        if( Abs( alphaTgt - alphaCur ) < kAlphaEps )
          alphaCur = alphaTgt;
        else
          Tac::Spring( &alphaCur, &alphaVel, alphaTgt, 32.0f, TAC_DT );

#if TAC_IMGUI_RESIZE_DEBUG()
        const bool drawBorder { true };
#else
        //const bool drawBorder { hovered || edgeActive || Abs( alphaTgt - alphaCur ) > kAlphaEps };
        const bool drawBorder{ true };
#endif

        //if( drawBorder )
        {
          dynmc v4 color{};

#if TAC_IMGUI_RESIZE_DEBUG()
          const v4 red{ 1, 0, 0, 1 };
          const v4 green{ 0, 1, 0, 1 };
          const v4 yellow{ 1, 1, 0, 1 };
          color = red;
          if( hovered )
            color = yellow;
          if( edgeActive )
            color = green;
#else
          const v4& framgBGColor{ImGuiGetColor( ImGuiCol::FrameBG )};
          if( edgeActive )
            color = v4( framgBGColor.xyz() * 3, alphaCur );
          else if( hovered )
            color = v4( framgBGColor.xyz() * 2, alphaCur );
          else
            color = v4( framgBGColor.xyz() * 1, alphaCur );
#endif
          mDrawData->AddBox(
            UI2DDrawData::Box
            {
              .mMini  { edgeRect_VS.mMini },
              .mMaxi  { edgeRect_VS.mMaxi},
              .mColor { color },
            } );
        }

        hoverMask |= hovered ? dir : 0;

        if( edgeActive )
        {
          switch( dir )
          {
          case E: targetWindowRect_VS.mMaxi.x = Max( targetWindowRect_VS.mMini.x + 50.0f, mousePos_VS.x + globals.mActiveIDWindowSize.x - globals.mActiveIDClickPos_VS.x ); break;
          case W: targetWindowRect_VS.mMini.x = Min( targetWindowRect_VS.mMaxi.x - 50.0f, mousePos_VS.x                                 - globals.mActiveIDClickPos_VS.x ); break;
          case N: targetWindowRect_VS.mMini.y = Min( targetWindowRect_VS.mMaxi.y - 50.0f, mousePos_VS.y                                 - globals.mActiveIDClickPos_VS.y ); break;
          case S: targetWindowRect_VS.mMaxi.y = Max( targetWindowRect_VS.mMini.y + 50.0f, mousePos_VS.y + globals.mActiveIDWindowSize.y - globals.mActiveIDClickPos_VS.y ); break;
          }
        }
    }

    if( anyEdgeActive && !UIKeyboardApi::IsPressed( Key::MouseLeft ) )
    {
      ClearActiveID();
    }

    if( !globals.mActiveID.IsValid() &&
        hoverMask &&
        UIKeyboardApi::JustPressed( Key::MouseLeft ) )
    {
      globals.mActiveIDClickPos_VS = mousePos_VS;
      globals.mActiveIDWindowSize = mSize;
      globals.mResizeMask = hoverMask;
      SetActiveID( id, this );
    }

    if( const int cursorMask{ anyEdgeActive ? globals.mResizeMask : hoverMask } )
    {
      ImGuiMouseCursor cursors[ 16 ]{};
      cursors[ N ] = ImGuiMouseCursor::kResizeNS;
      cursors[ E ] = ImGuiMouseCursor::kResizeEW;
      cursors[ W ] = ImGuiMouseCursor::kResizeEW;
      cursors[ S ] = ImGuiMouseCursor::kResizeNS;
      cursors[ N | E ] = ImGuiMouseCursor::kResizeNE_SW;
      cursors[ S | E ] = ImGuiMouseCursor::kResizeNW_SE;
      cursors[ S | W ] = ImGuiMouseCursor::kResizeNE_SW;
      cursors[ N | W ] = ImGuiMouseCursor::kResizeNW_SE;
      globals.mMouseCursor = cursors[ cursorMask ];
    }

    const v2 targetSize{ targetWindowRect_VS.GetSize() };
    const v2 targetViewportPos_SS{
      viewportPos_SS - origWindowRect_VS.mMini + targetWindowRect_VS.mMini };
    if( targetWindowRect_VS.mMini != origWindowRect_VS.mMini ||
        targetWindowRect_VS.mMaxi != origWindowRect_VS.mMaxi )
    {
      mDesktopWindow->mRequestedSize = targetSize;
      mDesktopWindow->mRequestedPosition = targetViewportPos_SS;

      globals.mSettingsDirty = true;
      mSize = targetSize;
    }
  }

  bool ImGuiWindow::Overlaps( const ImGuiRect& clipRect) const
  {
    return mViewportSpaceVisibleRegion.Overlaps( clipRect );
  }

  auto ImGuiWindow::Clip( const ImGuiRect& clipRect) const -> ImGuiRect 
  {
    return ImGuiRect
    {
      .mMini {  Max( clipRect.mMini, mViewportSpaceVisibleRegion.mMini )  },
      .mMaxi {  Min( clipRect.mMaxi, mViewportSpaceVisibleRegion.mMaxi )  },
    };
  }

  void ImGuiWindow::ItemSize( v2 size )
  {
    mCurrLineHeight = Max( mCurrLineHeight, size.y );
    UpdateMaxCursorDrawPos( mViewportSpaceCurrCursor + v2{ size.x, mCurrLineHeight } );

    mViewportSpacePrevCursor = mViewportSpaceCurrCursor + v2( size.x, 0 );
    mPrevLineHeight = mCurrLineHeight;

    mViewportSpaceCurrCursor.x = mViewportSpacePos.x + mXOffsets.back();
    mViewportSpaceCurrCursor.y += mCurrLineHeight + ImGuiGlobals::Instance.mUIStyle.itemSpacing.y;
    mCurrLineHeight = 0;
  }

  void ImGuiWindow::UpdateMaxCursorDrawPos( v2 pos )
  {
    mViewportSpaceMaxiCursor.x = Max( mViewportSpaceMaxiCursor.x, pos.x );
    mViewportSpaceMaxiCursor.y = Max( mViewportSpaceMaxiCursor.y, pos.y );
  }

  bool ImGuiWindow::IsHovered( const ImGuiRect& rectViewport, ImGuiID id )
  {
    if( ImGuiGlobals::Instance.mHoveredID && ImGuiGlobals::Instance.mHoveredID != id )
      return false;

    const WindowHandle mouseHoveredWindow { ImGuiGlobals::Instance.mMouseHoveredWindow };
    if( !mouseHoveredWindow.IsValid() )
      return false;

    const WindowHandle windowHandle { GetWindowHandle() };
    if( mouseHoveredWindow.GetIndex() != windowHandle.GetIndex() )
      return false;

    if( !rectViewport.ContainsPoint( GetMousePosViewport() ) )
      return false;

    if( id != ImGuiGlobals::Instance.mHoveredIDPrev )
      ImGuiGlobals::Instance.mHoverStartTime = ImGuiGlobals::Instance.mElapsedSeconds;

    ImGuiGlobals::Instance.mHoveredID = id;
    return true;
  }

  void ImGuiWindow::PushXOffset()
  {
    mXOffsets.push_back( mViewportSpaceCurrCursor.x - mViewportSpacePos.x );
  }

  auto ImGuiWindow::GetRemainingHeight() const -> float
  {
    return mViewportSpaceVisibleRegion.mMaxi.y - mViewportSpaceCurrCursor.y;
  }

  auto ImGuiWindow::GetRemainingWidth() const -> float
  {
    return mViewportSpaceVisibleRegion.mMaxi.x - mViewportSpaceCurrCursor.x;
  }

  auto ImGuiWindow::GetWindowHandle() const -> WindowHandle
  {
    return mDesktopWindow->mWindowHandle;
  }

  auto ImGuiWindow::GetWindowPosScreenspace() const -> v2
  {
    //ImGuiGlobals& globals { ImGuiGlobals::Instance };
     
    const v2 viewportPosScreenspace{ AppWindowApi::GetPos( mDesktopWindow->mWindowHandle ) };
    return viewportPosScreenspace + mViewportSpacePos;
  }

  auto ImGuiWindow::GetMousePosViewport() -> v2
  {
    const v2 mousePos_SS{ UIKeyboardApi::GetMousePosScreenspace() };
    const v2 windowPos_SS{ GetWindowPosScreenspace() };
    return mousePos_SS - windowPos_SS;
  }

  auto ImGuiWindow::GetID( StringView s ) -> ImGuiID
  {
    return Hash( mIDStack.back(), Hash( s ) );
  }

  auto ImGuiWindow::GetWindowResource( ImGuiRscIdx index, ImGuiID imGuiId ) -> void*
  {
    for( ImGuiWindowResource& resource : mResources )
      if( resource.mImGuiID == imGuiId && resource.mIndex == index )
        return resource.mData.data();

    WindowResourceRegistry* registry{ WindowResourceRegistry::GetInstance() };
    RegisteredWindowResource* pRegistered { registry->FindResource( index ) };
    TAC_ASSERT( pRegistered );

    mResources.resize( mResources.size() + 1 );
    ImGuiWindowResource& resource { mResources.back() };
    resource = ImGuiWindowResource
    {
      .mImGuiID { imGuiId },
      .mIndex   { index },
      .mData    { pRegistered->mInitialData },
    };
    return resource.mData.data();
  }

  auto ImGuiGlobals::FindDesktopWindow( WindowHandle h ) -> ImGuiDesktopWindowImpl*
  {
    for( ImGuiDesktopWindowImpl* impl : mDesktopWindows )
      if( impl->mWindowHandle.GetIndex() == h.GetIndex() )
        return impl;

    return nullptr;
  }

  auto ImGuiGlobals::FindWindow( const StringView& name ) -> ImGuiWindow*
  {
    for( ImGuiWindow* window : mAllWindows )
      if( ( StringView )window->mName == name )
        return window;

    return nullptr;
  }

  // -----------------------------------------------------------------------------------------------

  static void UpdateAndRenderWindow( ImGuiDesktopWindowImpl* desktopWindow, Errors& errors )
  {
    if( !AppWindowApi::IsShown( desktopWindow->mWindowHandle ) )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Span< UI2DDrawData* > drawDatas( desktopWindow->mImGuiDrawDatas.data(), desktopWindow->mImGuiDrawDatas.size() );
    const Render::SwapChainHandle fb { AppWindowApi::GetSwapChainHandle( desktopWindow->mWindowHandle ) };
    const Render::TextureHandle swapChainColor { renderDevice->GetSwapChainCurrentColor( fb ) };
    const Render::TexFmt fbFmt { renderDevice->GetSwapChainColorFmt( fb ) };
    const v2i windowSize { AppWindowApi::GetSize( desktopWindow->mWindowHandle ) };
    TAC_CALL( desktopWindow->mRenderBuffers.DebugDraw2DToTexture( drawDatas, swapChainColor, fbFmt, windowSize, errors ) );
  }

  void Tac::ImGuiPlatformRender( Errors& errors )
  {
    ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    for( ImGuiDesktopWindowImpl* desktopWindow : globals.mDesktopWindows )
    {
      desktopWindow->mImGuiDrawDatas.clear();

      // Grab all imgui windows that use this viewport window
      for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
      {
        // All windows of the same viewport share the same draw data
        if( window->mDesktopWindow == desktopWindow && !window->mParent )
        {
          desktopWindow->mImGuiDrawDatas.push_back( window->mDrawData );
          window->mDrawData->mDebugGroupStack.AssertNodeHeights();
        }
      }
    }

    for( ImGuiDesktopWindowImpl* desktopWindow : globals.mDesktopWindows )
    {
      TAC_CALL( UpdateAndRenderWindow( desktopWindow, errors ) );
    }
  }

  // -----------------------------------------------------------------------------------------------

  auto ImGuiWindowResource::Register( Params params ) -> ImGuiRscIdx
  {
    return WindowResourceRegistry::GetInstance()->RegisterResource( params );
  }


} // namespace Tac

auto Tac::GetActiveID() -> Tac::ImGuiID { return ImGuiGlobals::Instance.mActiveID; }

void Tac::SetActiveID( ImGuiID id, ImGuiWindow* window )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  globals.mActiveID = id;
  globals.mActiveIDWindow = window;
  globals.mActiveIDClickPos_VS = window->GetMousePosViewport();
  globals.mActiveIDWindowPos_SS = window->GetWindowPosScreenspace();
}

void Tac::ClearActiveID()
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  globals.mActiveID = {};
  globals.mActiveIDWindow = {};
  globals.mActiveIDClickPos_VS = {};
  globals.mActiveIDWindowPos_SS = {};
}

