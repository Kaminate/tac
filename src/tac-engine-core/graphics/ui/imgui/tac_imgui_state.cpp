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

  ImGuiMouseCursor                  ImGuiGlobals::mMouseCursor          {};
  GameTime                          ImGuiGlobals::mElapsedSeconds       {};
  Vector< ImGuiWindow* >            ImGuiGlobals::mAllWindows           {};
  Vector< String >                  ImGuiGlobals::mPopupStack           {};
  Vector< ImGuiWindow* >            ImGuiGlobals::mWindowStack          {};
  Vector< ImGuiDesktopWindowImpl* > ImGuiGlobals::mDesktopWindows       {};
  ImGuiWindow*                      ImGuiGlobals::mCurrentWindow        {};
  Vector< float >                   ImGuiGlobals::mFontSizeStack        {};
  UIStyle                           ImGuiGlobals::mUIStyle              {};
  WindowHandle                      ImGuiGlobals::mMouseHoveredWindow   {};
  bool                              ImGuiGlobals::mScrollBarEnabled     { true };
  int                               ImGuiGlobals::mMaxGpuFrameCount     {};
  SettingsNode                      ImGuiGlobals::mSettingsNode         {};
  ImGuiID                           ImGuiGlobals::mHoveredID            {};
  ImGuiID                           ImGuiGlobals::mHoveredIDPrev        {};
  GameTime                          ImGuiGlobals::mHoverStartTime       {};
  ImGuiID                           ImGuiGlobals::mActiveID             {};
  ImGuiWindow*                      ImGuiGlobals::mActiveIDWindow       {};
  ImGuiWindow*                      ImGuiGlobals::mMovingWindow         {};
  v2                                ImGuiGlobals::mActiveIDClickPos_UIWindowSpace {};
  v2                                ImGuiGlobals::mActiveIDWindowSize   {};
  v2                                ImGuiGlobals::mActiveIDWindowPos_DesktopSpace {};
  int                               ImGuiGlobals::mResizeMask           {};
  bool                              ImGuiGlobals::mSettingsDirty        {};
  ReferenceResolution               ImGuiGlobals::mReferenceResolution  {};

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
    const bool scrollBarEnabled { ImGuiGlobals::mScrollBarEnabled };
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
    if( !ImGuiGlobals::GetActiveID().IsValid()
        && ImGuiGlobals::mMouseHoveredWindow == GetWindowHandle()
        && UIKeyboardApi::GetMouseWheelDelta()
        && ImGuiRect::FromPosSize( mViewportSpacePos, mSize ).ContainsPoint( GetMousePos_uiwindowspace() ) )
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
        ImGuiGlobals::SetActiveID( id, this );
        mScrollMousePosScreenspaceInitial = UIKeyboardApi::GetMousePosScreenspace();
      }
    }

    const float scrollbarHeight { scrollbarForegroundRect.GetHeight() };
    const bool active { ImGuiGlobals::GetActiveID() == id };

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


    if( active )
    {
      const float mouseDY{
        UIKeyboardApi::GetMousePosScreenspace().y
       - mScrollMousePosScreenspaceInitial.y };
      mScrollMousePosScreenspaceInitial.y = (float)UIKeyboardApi::GetMousePosScreenspace().y;
      const float scrollDY{ mouseDY * ( contentVisibleHeight / scrollbarHeight ) };
      mScroll = Clamp( mScroll + scrollDY , scrollMin, scrollMax );
      if( !UIKeyboardApi::IsPressed( Key::MouseLeft ) )
        ImGuiGlobals::ClearActiveID();
    }

    mViewportSpaceVisibleRegion.mMaxi.x -= scrollbarWidth;
  }

  void ImGuiWindow::DrawWindowBackground()
  {
    if( const bool drawWindow{ mEnableBG
        && ( mParent || mStretchWindow || mWindowHandleOwned ) } )
    {
      if( const ImGuiRect origRect { ImGuiRect::FromPosSize( mViewportSpacePos, mSize ) };
          Overlaps( origRect ) )
      {
        const ImGuiRect clipRect { Clip( origRect ) };
        const ImGuiCol col{ mParent ? ImGuiCol::ChildWindowBackground : ImGuiCol::WindowBackground };
        mDrawData->AddBox(
          UI2DDrawData::Box
          {
            .mMini  { clipRect.mMini },
            .mMaxi  { clipRect.mMaxi },
            .mColor { ImGuiGetColor( col ) },
          }, &clipRect );
      }
    }
  }

  void ImGuiWindow::EndFrame()
  {
    mDrawData->PopDebugGroup(); // pushed in ImGuiWindow::BeginFrame
    if( ( mFlags & ImGuiWindowFlags_AutoResize ) )
    {
      if( const v2 tgtSize{ mViewportSpaceMaxiCursor + v2( 1, 1 ) * ImGuiGetWindowPaddingPx() };
          mSize != tgtSize )
      {
        mDesktopWindow->mRequestedSize = tgtSize;
        mSize = tgtSize;
      }
    }
  }

  void ImGuiWindow::BeginFrame()
  {
    const float windowPaddingPx{ ImGuiGetWindowPaddingPx() };

    if( mParent )
    {
      mDesktopWindow = mParent->mDesktopWindow;
      mWindowHandleOwned = false;
    }
    else
    {
      mDrawData->clear();
    }

    // popped in ImGuiWindow::EndFrame
    mDrawData->PushDebugGroup( ShortFixedString::Concat( "BeginFrame(" , mName , ")" ) );
    mIDStack.clear();
    mIDStack.push_back( mWindowID = Hash( mName ) );
    mMoveID = GetID( "#MOVE" );
    mViewportSpacePos = mParent ? mParent->mViewportSpaceCurrCursor : mViewportSpacePos;
    mViewportSpaceVisibleRegion = ImGuiRect::FromPosSize( mViewportSpacePos, mSize );
    mViewportSpaceCurrCursor = mViewportSpaceVisibleRegion.mMini;
    mViewportSpacePrevCursor = mViewportSpaceCurrCursor;
    mViewportSpaceMaxiCursor = mViewportSpaceCurrCursor;
    mXOffsets.clear();
    PushXOffset();
    DrawWindowBackground();
    Scrollbar();
    ResizeControls();
    MenuBar();
    mViewportSpaceVisibleRegion.mMini += v2( 1, 1 ) * windowPaddingPx;
    mViewportSpaceVisibleRegion.mMaxi -= v2( 1, 1 ) * windowPaddingPx;
    mViewportSpaceCurrCursor = mViewportSpaceVisibleRegion.mMini - v2( 0, mScroll ) ;
    mViewportSpacePrevCursor = mViewportSpaceCurrCursor;
    mViewportSpaceMaxiCursor = mViewportSpaceCurrCursor;
    PushXOffset();
    mCurrLineHeight = 0;
    mPrevLineHeight = 0;

    if( !( mFlags & ImGuiWindowFlags_NoBorder ) )
      mDrawData->AddBoxOutline(
        UI2DDrawData::Box
        {
          .mMini  { mViewportSpacePos },
          .mMaxi  { mViewportSpacePos + mSize },
          .mColor { 1, 1, 0, .2f },
        } );
  }

  void ImGuiWindow::ToggleFullscreen()
  {
    const v2i windowSize{ AppWindowApi::GetSize( mDesktopWindow->mWindowHandle ) };
    const v2i windowPos{ AppWindowApi::GetPos( mDesktopWindow->mWindowHandle ) };
    const void* nwh{ AppWindowApi::GetNativeWindowHandle( mDesktopWindow->mWindowHandle ) };
    const Monitor monitor{ OS::OSGetMonitorFromNativeWindowHandle( nwh ) };
    const bool isFullscreen{ windowPos == monitor.mPos && windowSize == monitor.mSize };
    if( isFullscreen )
    {
      mSize = mPreviousViewportToggleSize;
      mDesktopWindow->mRequestedPosition = mPreviousViewportTogglePos;
      mDesktopWindow->mRequestedSize = mPreviousViewportToggleSize;
    }
    else
    {
      mPreviousViewportTogglePos = windowPos;
      mPreviousViewportToggleSize = mSize;
      mSize = monitor.mSize;
      mDesktopWindow->mRequestedPosition = monitor.mPos;
      mDesktopWindow->mRequestedSize = monitor.mSize;
    }
  }

  void ImGuiWindow::MenuBar()
  {
    if( mParent )
      return;

    if( mFlags & ImGuiWindowFlags_NoTitleBar )
      return;

    const v2 itemSpacingPx{ ImGuiGetItemSpacingPx() };
    const float fontSizePx{ ImGuiGetFontSizePx() };
    const float buttonPadPx{ ImGuiGetButtonPaddingPx() };


    //const float menuBarY{ mViewportSpaceCurrCursor.y };
    ImGuiText( mName );
    ImGuiSameLine();

    mMenuBarMini.x = mViewportSpaceMaxiCursor.x;
    mMenuBarMini.y = mViewportSpaceCurrCursor.y;

    v2 moveWindowGrabbableMini{ mMenuBarMini + v2( mMenuBarPrevWidth, 0 ) };
    v2 moveWindowGrabbableMaxi{ mViewportSpaceVisibleRegion.mMaxi.x, mViewportSpaceCurrCursor.y + fontSizePx};

    // Title Bar buttons (minimize, maximize, close)
    if( mOpen )
    {
      const char* minimizeStr{" - "};
      const char* fullscrStr{" [] "};
      const char* closeStr{" X "};
      const v2 minimizeStrSize{ CalculateTextSize( minimizeStr, fontSizePx ) };
      const v2 fullscrStrSize{ CalculateTextSize( fullscrStr, fontSizePx ) };
      const v2 closeStrSize{ CalculateTextSize( closeStr, fontSizePx ) };
      const float titleBarButtonsWidthPx{ 0
        + minimizeStrSize.x + buttonPadPx * 2
        + fullscrStrSize.x + buttonPadPx * 2
        + closeStrSize.x + buttonPadPx * 2 };

      mViewportSpaceCurrCursor.x = mViewportSpaceVisibleRegion.mMaxi.x - titleBarButtonsWidthPx;
      moveWindowGrabbableMaxi.x = mViewportSpaceVisibleRegion.mMaxi.x - titleBarButtonsWidthPx;
      moveWindowGrabbableMaxi.x = Max( moveWindowGrabbableMaxi.x, moveWindowGrabbableMini.x );

      if( ImGuiButton( minimizeStr ) )
        AppWindowApi::MinimizeWindow( mDesktopWindow->mWindowHandle );
      ImGuiSameLine();
      mViewportSpaceCurrCursor.x -= itemSpacingPx.x;
      if( ImGuiButton( fullscrStr ) )
        ToggleFullscreen();
      ImGuiSameLine();
      mViewportSpaceCurrCursor.x -= itemSpacingPx.x;
      if( ImGuiButton( closeStr ) )
        *mOpen = !*mOpen;
      ImGuiSameLine();
    }

    mDrawData->AddBox(
      UI2DDrawData::Box
      {
        .mMini  { moveWindowGrabbableMini },
        .mMaxi  { moveWindowGrabbableMaxi },
        .mColor { ImGuiGetColor( ImGuiCol::ChildWindowBackground ) },
      }
    );

    //if( mViewportSpaceCurrCursor.y != menuBarY )
    //  mViewportSpaceCurrCursor.y -= ImGuiGetItemSpacingPx().y;

    ItemSize( v2( 0, 0 ) ); // advance to next line

    mViewportSpaceVisibleRegion.mMini = mViewportSpaceCurrCursor;
    mViewportSpaceMouseGrabRegion = ImGuiRect::FromMinMax( moveWindowGrabbableMini, moveWindowGrabbableMaxi );
  }

  void ImGuiWindow::UpdateMoveControls()
  {

    if( UIKeyboardApi::IsPressed( Key::MouseLeft ) )
    {
      mDesktopWindow->mRequestedPosition = GetMousePos_uiwindowspace()
                                         - ImGuiGlobals::mActiveIDClickPos_UIWindowSpace
                                         + mViewportSpacePos
                                         + GetWindowPos_desktopspace();


      ImGuiGlobals::mMouseCursor = ImGuiMouseCursor::kResizeNS_EW;
      ImGuiGlobals::mSettingsDirty = true;
    }
    else
    {
      ImGuiGlobals::mMovingWindow = nullptr;
      ImGuiGlobals::ClearActiveID();
    }
  }

  void ImGuiWindow::BeginMoveControls()
  {
    if( ImGuiGlobals::mActiveID.IsValid() )
      return;

    if( mDesktopWindow->mWindowHandle != ImGuiGlobals::mMouseHoveredWindow )
      return;

    if( IsHovered( mViewportSpaceMouseGrabRegion, mMoveID ) )
      ImGuiGlobals::mMouseCursor = ImGuiMouseCursor::kResizeNS_EW;
    else
      return;

    if( !UIKeyboardApi::JustPressed( Key::MouseLeft ) )
      return;
    
    ImGuiGlobals::SetActiveID( mMoveID, this );
    ImGuiGlobals::mActiveIDClickPos_UIWindowSpace = GetMousePos_uiwindowspace();
    ImGuiGlobals::mActiveIDWindowPos_DesktopSpace = GetWindowPos_desktopspace();
    ImGuiGlobals::mMovingWindow = this;
  }

  void ImGuiWindow::ResizeControls()
  {
    if( mParent )
      return;

    if( mFlags & ImGuiWindowFlags_NoResize )
      return;

    const ImGuiID id{ GetID( "##RESIZE" ) };
    const UIStyle style{ ImGuiGlobals::mUIStyle };
    const float windowPaddingPx{ ImGuiGetWindowPaddingPx() };
    const v2i viewportPos_SS{ AppWindowApi::GetPos( mDesktopWindow->mWindowHandle ) };
    const v2 mousePos_VS{ GetMousePos_uiwindowspace() };
    const ImGuiRect origWindowRect_VS{ ImGuiRect::FromPosSize( mViewportSpacePos, mSize ) };
    dynmc ImGuiRect targetWindowRect_VS{ origWindowRect_VS };
    const int E { 1 << 0 };
    const int S { 1 << 1 };
    const int W { 1 << 2 };
    const int N { 1 << 3 };
    dynmc int hoverMask {};
    const bool anyEdgeActive{ ImGuiGlobals::mActiveID == id };

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
        const bool edgeActive{ anyEdgeActive && ( ImGuiGlobals::mResizeMask & dir ) };
        dynmc float& alphaCur{ mBorderData[ iSide ].x };
        dynmc float& alphaVel{ mBorderData[ iSide ].y };
        dynmc float alphaTgt = 0.05f;
        if( edgeActive ) { alphaTgt += .6f; }
        else if( hovered ) {
          const GameTimeDelta hoverTime{
            ImGuiGlobals::mElapsedSeconds -
            ImGuiGlobals::mHoverStartTime };
          alphaTgt += 0.2f * ( .5f + .5f * ( float )Sin( -3.14f / 2 + hoverTime.mSeconds * 4 ) );
        }
        
        const float kAlphaEps{ 0.01f };
        if( Abs( alphaTgt - alphaCur ) < kAlphaEps )
          alphaCur = alphaTgt;
        else
          Tac::Spring(
            SpringParams{
              .mPosCur           { &alphaCur },
              .mVelCur           { &alphaVel },
              .mPosTgt           { alphaTgt },
              .mSpringyness      { 32.0f },
              .mDeltaTimeSeconds { TAC_DT } } );

        //const bool drawBorder{ true };

        //if( drawBorder )
        {
          dynmc v4 color{ImGuiGetColor( ImGuiCol::ResizeGrip )};
          if( edgeActive )
            color.xyz() *= 3 * alphaCur;
          else if( hovered )
            color.xyz() *= 2 * alphaCur;
          else
            color.xyz() *= alphaCur;
          

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
            case E:
              targetWindowRect_VS.mMaxi.x = Max(
                targetWindowRect_VS.mMini.x + 50.0f,
                mousePos_VS.x + ImGuiGlobals::mActiveIDWindowSize.x - ImGuiGlobals::mActiveIDClickPos_UIWindowSpace.x );
              break;
            case W:
              targetWindowRect_VS.mMini.x = Min(
                targetWindowRect_VS.mMaxi.x - 50.0f,
                mousePos_VS.x - ImGuiGlobals::mActiveIDClickPos_UIWindowSpace.x );
              break;
            case N:
              targetWindowRect_VS.mMini.y = Min(
                targetWindowRect_VS.mMaxi.y - 50.0f,
                mousePos_VS.y - ImGuiGlobals::mActiveIDClickPos_UIWindowSpace.y );
              break;
            case S:
              targetWindowRect_VS.mMaxi.y = Max(
                targetWindowRect_VS.mMini.y + 50.0f,
                mousePos_VS.y + ImGuiGlobals::mActiveIDWindowSize.y - ImGuiGlobals::mActiveIDClickPos_UIWindowSpace.y );
              break;
          }
        }
    }


    if( anyEdgeActive && !UIKeyboardApi::IsPressed( Key::MouseLeft ) )
    {
      ImGuiGlobals::ClearActiveID();
    }

    if( !ImGuiGlobals::mActiveID.IsValid() &&
        hoverMask &&
        UIKeyboardApi::JustPressed( Key::MouseLeft ) )
    {
      ImGuiGlobals::mActiveIDClickPos_UIWindowSpace = mousePos_VS;
      ImGuiGlobals::mActiveIDWindowSize = mSize;
      ImGuiGlobals::mResizeMask = hoverMask;
      ImGuiGlobals::SetActiveID( id, this );
    }

    if( const int cursorMask{ anyEdgeActive ? ImGuiGlobals::mResizeMask : hoverMask } )
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
      cursors[ N | S | E | W ] = ImGuiMouseCursor::kResizeNS_EW;
      ImGuiGlobals::mMouseCursor = cursors[ cursorMask ];
    }

    const v2 targetSize{ targetWindowRect_VS.GetSize() };
    const v2 targetViewportPos_SS{
      viewportPos_SS - origWindowRect_VS.mMini + targetWindowRect_VS.mMini };
    if( targetWindowRect_VS.mMini != origWindowRect_VS.mMini ||
        targetWindowRect_VS.mMaxi != origWindowRect_VS.mMaxi )
    {
      mDesktopWindow->mRequestedSize = targetSize;
      mDesktopWindow->mRequestedPosition = targetViewportPos_SS;

      ImGuiGlobals::mSettingsDirty = true;
      mSize = targetSize;
    }

    mViewportSpaceVisibleRegion.mMini += v2( 1, 1 ) * windowPaddingPx;
    mViewportSpaceVisibleRegion.mMaxi -= v2( 1, 1 ) * windowPaddingPx;
    mViewportSpaceCurrCursor = mViewportSpaceVisibleRegion.mMini;
    PushXOffset();
  }

  bool ImGuiWindow::Overlaps( const ImGuiRect& clipRect) const
  {
    if( mAppendingToMenuBar ) { return true; } // menu items are outside content rect
    return mViewportSpaceVisibleRegion.Overlaps( clipRect );
  }

  auto ImGuiWindow::Clip( const ImGuiRect& clipRect) const -> ImGuiRect 
  {
    if( mAppendingToMenuBar ) { return clipRect; } // menu items are outside content rect
    return ImGuiRect
    {
      .mMini { Max( clipRect.mMini, mViewportSpaceVisibleRegion.mMini ) },
      .mMaxi { Min( clipRect.mMaxi, mViewportSpaceVisibleRegion.mMaxi ) },
    };
  }

  void ImGuiWindow::ItemSize( v2 size )
  {
    mCurrLineHeight = Max( mCurrLineHeight, size.y );
    UpdateMaxCursorDrawPos( mViewportSpaceCurrCursor + v2{ size.x, mCurrLineHeight } );
    mViewportSpacePrevCursor = mViewportSpaceCurrCursor + v2( size.x, 0 );
    mPrevLineHeight = mCurrLineHeight;
    mViewportSpaceCurrCursor.x = mViewportSpacePos.x + mXOffsets.back();
    mViewportSpaceCurrCursor.y += mCurrLineHeight + ImGuiGetItemSpacingPx().y;
    mCurrLineHeight = 0;
  }

  void ImGuiWindow::UpdateMaxCursorDrawPos( v2 pos )
  {
    mViewportSpaceMaxiCursor.x = Max( mViewportSpaceMaxiCursor.x, pos.x );
    mViewportSpaceMaxiCursor.y = Max( mViewportSpaceMaxiCursor.y, pos.y );
  }

  bool ImGuiWindow::IsHovered( const ImGuiRect& rectViewport, ImGuiID id ) const
  {
    if( ImGuiGlobals::mHoveredID && ImGuiGlobals::mHoveredID != id )
      return false;

    const WindowHandle mouseHoveredWindow { ImGuiGlobals::mMouseHoveredWindow };
    const WindowHandle windowHandle { GetWindowHandle() };
    if( mouseHoveredWindow.GetIndex() != windowHandle.GetIndex() )
      return false;

    if( !rectViewport.ContainsPoint( GetMousePos_nwhspace() ) )
      return false;

    if( id != ImGuiGlobals::mHoveredIDPrev )
      ImGuiGlobals::mHoverStartTime = ImGuiGlobals::mElapsedSeconds;

    ImGuiGlobals::mHoveredID = id;
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

  auto ImGuiWindow::GetWindowPos_desktopspace() const -> v2
  {
    const v2 nwh_screenspace{ AppWindowApi::GetPos( mDesktopWindow->mWindowHandle ) };
    return nwh_screenspace + mViewportSpacePos;
  }

  auto ImGuiWindow::GetMousePos_nwhspace() const -> v2
  {
    const v2 mousePos_SS{ UIKeyboardApi::GetMousePosScreenspace() };
    const v2 nwh_SS{ AppWindowApi::GetPos( mDesktopWindow->mWindowHandle ) };
    return mousePos_SS - nwh_SS;
  }

  auto ImGuiWindow::GetMousePos_uiwindowspace() const -> v2
  {
    const v2 mousePos_SS{ UIKeyboardApi::GetMousePosScreenspace() };
    const v2 windowPos_SS{ GetWindowPos_desktopspace() };
    return mousePos_SS - windowPos_SS;
  }

  auto ImGuiWindow::GetID( StringView s ) const -> ImGuiID
  {
    return Hash( mIDStack.back().mValue, Hash( s ) );
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

  // -----------------------------------------------------------------------------------------------

  auto ImGuiGlobals::FindDesktopWindow( WindowHandle h ) -> ImGuiDesktopWindowImpl*
  {
    for( ImGuiDesktopWindowImpl* impl : mDesktopWindows )
      if( impl->mWindowHandle.GetIndex() == h.GetIndex() )
        return impl;

    return nullptr;
  }

  auto ImGuiGlobals::FindWindow( const StringView name ) -> ImGuiWindow*
  {
    for( ImGuiWindow* window : mAllWindows )
      if( ( StringView )window->mName == name )
        return window;

    return nullptr;
  }

  auto ImGuiGlobals::GetActiveID() -> Tac::ImGuiID { return ImGuiGlobals::mActiveID; }

  void ImGuiGlobals::SetActiveID( ImGuiID id, ImGuiWindow* window )
  {
    ImGuiGlobals::mActiveID = id;
    ImGuiGlobals::mActiveIDWindow = window;
    ImGuiGlobals::mActiveIDClickPos_UIWindowSpace = window->GetMousePos_uiwindowspace();
    ImGuiGlobals::mActiveIDWindowPos_DesktopSpace = window->GetWindowPos_desktopspace();
  }

  void ImGuiGlobals::ClearActiveID()
  {
    ImGuiGlobals::mActiveID = {};
    ImGuiGlobals::mActiveIDWindow = {};
    ImGuiGlobals::mActiveIDClickPos_UIWindowSpace = {};
    ImGuiGlobals::mActiveIDWindowPos_DesktopSpace = {};
  }


  // -----------------------------------------------------------------------------------------------

  static void UpdateAndRenderWindow( Render::IContext* renderContext, ImGuiDesktopWindowImpl* desktopWindow, Errors& errors )
  {
    if( !AppWindowApi::IsShown( desktopWindow->mWindowHandle ) )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Span< UI2DDrawData* > drawDatas( desktopWindow->mImGuiDrawDatas.data(), desktopWindow->mImGuiDrawDatas.size() );
    const Render::SwapChainHandle fb { AppWindowApi::GetSwapChainHandle( desktopWindow->mWindowHandle ) };
    const Render::TextureHandle swapChainColor { renderDevice->GetSwapChainCurrentColor( fb ) };
    const Render::TexFmt fbFmt { renderDevice->GetSwapChainColorFmt( fb ) };
    const v2i windowSize { AppWindowApi::GetSize( desktopWindow->mWindowHandle ) };
    TAC_CALL( desktopWindow->mRenderBuffers.DebugDraw2DToTexture( renderContext, drawDatas, swapChainColor, fbFmt, windowSize, errors ) );
  }

  void Tac::ImGuiPlatformRender( Errors& errors )
  {
    
    for( ImGuiDesktopWindowImpl* desktopWindow : ImGuiGlobals::mDesktopWindows )
    {
      desktopWindow->mImGuiDrawDatas.clear();

      // Grab all imgui windows that use this viewport window
      for( ImGuiWindow* window : ImGuiGlobals::mAllWindows )
      {
        // All windows of the same viewport share the same draw data
        if( window->mDesktopWindow == desktopWindow && !window->mParent )
        {
          desktopWindow->mImGuiDrawDatas.push_back( window->mDrawData );
          window->mDrawData->mDebugGroupStack.AssertNodeHeights();
        }
      }
    }

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( Render::IContext::Scope renderContextScope{ renderDevice->CreateRenderContext( errors ) } );
    Render::IContext* renderContext{ renderContextScope.GetContext() };
    renderContext->DebugEventBegin( "ImGuiPlatformRender()" );
    for( ImGuiDesktopWindowImpl* desktopWindow : ImGuiGlobals::mDesktopWindows )
    {
      TAC_CALL( UpdateAndRenderWindow( renderContext, desktopWindow, errors ) );
    }
    renderContext->DebugEventEnd();
    renderContext->Execute( errors );
  }

  // -----------------------------------------------------------------------------------------------

  auto ImGuiWindowResource::Register( Params params ) -> ImGuiRscIdx
  {
    return WindowResourceRegistry::GetInstance()->RegisterResource( params );
  }


} // namespace Tac
