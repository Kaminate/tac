#include "tac_imgui_state.h" // self-inc

#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/tac_text_edit.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
//#include "tac-engine-core/window/tac_window_backend.h"

#include "tac-rhi/render3/tac_render_api.h" // CreateContext

#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

#define TAC_IMGUI_RESIZE_DEBUG() TAC_IS_DEBUG_MODE() && false

Tac::ImGuiNextWindow          Tac::gNextWindow;

namespace Tac
{

  struct PerFrameType
  {
    m4    mOrthoProj         {};
    float mSDFOnEdge         {};
    float mSDFPixelDistScale {};
  };

  struct PerObjectType
    {
      v4  mColor{};
      u32 mType{};
    };

  ImGuiPersistantPlatformData ImGuiPersistantPlatformData::Instance;
  ImGuiGlobals                ImGuiGlobals::Instance;

  // -----------------------------------------------------------------------------------------------

  struct RegisteredWindowResource
  {
    String         mName;
    Vector< char > mInitialData;
    ImGuiRscIdx     mId {};
  };

  // -----------------------------------------------------------------------------------------------

  struct WindowResourceRegistry
  {
    static WindowResourceRegistry*     GetInstance();
    ImGuiRscIdx                         RegisterResource( StringView name,
                                                         const void* initialDataBytes,
                                                         int initialDataByteCount );
    RegisteredWindowResource*          FindResource( ImGuiRscIdx index );
  private:
    Vector< RegisteredWindowResource > mRegisteredWindowResources;
    int                                mResourceCounter {};
  };

  // -----------------------------------------------------------------------------------------------

  RegisteredWindowResource* WindowResourceRegistry::FindResource( ImGuiRscIdx index )
  {
    for( RegisteredWindowResource& resource : mRegisteredWindowResources )
      if( resource.mId == index )
        return &resource;
    return nullptr;
  }

  WindowResourceRegistry*   WindowResourceRegistry::GetInstance()
  {
    static WindowResourceRegistry instance;
    return &instance;
  }

  ImGuiRscIdx               WindowResourceRegistry::RegisterResource( StringView name,
                                                                      const void* initialDataBytes,
                                                                      int initialDataByteCount )
  {
    Vector< char > initialData;
    if( initialDataBytes )
    {
      const char* dataBegin { ( char* )initialDataBytes };
      const char* dataEnd { ( char* )initialDataBytes + initialDataByteCount };
      initialData.assign( dataBegin, dataEnd );
    }
    else
    {
      initialData.assign( initialDataByteCount, 0 );
    }
    const ImGuiRscIdx id { mResourceCounter++ };
    const RegisteredWindowResource resource
    {
      .mName        { name },
      .mInitialData { initialData },
      .mId          { id },
    };
    mRegisteredWindowResources.push_back( resource );
    return id;
  }

  // -----------------------------------------------------------------------------------------------

  ImGuiWindow::ImGuiWindow()
  {
  }

  ImGuiWindow::~ImGuiWindow()
  {
  }

  void         ImGuiWindow::Scrollbar()
  {
    ImGuiGlobals& globals { ImGuiGlobals::Instance };

    const bool scrollBarEnabled { globals.mScrollBarEnabled };
    if( !scrollBarEnabled )
      return;

    SimKeyboardApi keyboardApi { globals.mSimKeyboardApi };

    const bool stuffBelowScreen { mViewportSpaceMaxiCursor.y > mViewportSpaceVisibleRegion.mMaxi.y };
    const bool stuffAboveScreen{ ( bool )mScroll };
    if( !stuffBelowScreen && !stuffAboveScreen )
      return;

    mDrawData->PushDebugGroup( "scrollbar" );
    TAC_ON_DESTRUCT( mDrawData->PopDebugGroup() );

    const float scrollbarWidth { 30 };


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
        && IsHovered( ImGuiRect::FromPosSize( mViewportSpacePos, mSize ) )
        && keyboardApi.GetMouseWheelDelta() )
      mScroll = Clamp( mScroll - keyboardApi.GetMouseWheelDelta() * 40.0f, scrollMin, scrollMax );

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
    const ImGuiID id{ GetID( "##SCROLLBAR" ) };

    const bool hovered { IsHovered( scrollbarForegroundRect ) };
    if( hovered )
    {
      SetHoveredID( id );

      if( keyboardApi.JustPressed( Key::MouseLeft ) )
      {
        SetActiveID( id, this );
        mScrollMousePosScreenspaceInitial = keyboardApi.GetMousePosScreenspace();
      }
    }

    const float scrollbarHeight { scrollbarForegroundRect.GetHeight() };
    const bool active { GetActiveID() == id };

    v4 barColor{ ImGuiGetColor( ImGuiCol::Scrollbar ) };
    if( hovered )
      barColor =  ImGuiGetColor( ImGuiCol::ScrollbarHovered );
    if( active )
      barColor =  ImGuiGetColor( ImGuiCol::ScrollbarActive );

    const UI2DDrawData::Box scrollbarBox
    {
      .mMini  { scrollbarForegroundMini },
      .mMaxi  { scrollbarForegroundMaxi },
      .mColor { barColor },
    };

    mDrawData->AddBox( scrollbarBox );

    //static Timestamp consumeT;
    //if( active )
    //  Mouse::TryConsumeMouseMovement( &consumeT, TAC_STACK_FRAME );


    if(active)
    {
      const float mouseDY{
        keyboardApi.GetMousePosScreenspace().y
       - mScrollMousePosScreenspaceInitial.y };
      mScrollMousePosScreenspaceInitial.y = (float)keyboardApi.GetMousePosScreenspace().y;
      const float scrollDY{ mouseDY * ( contentVisibleHeight / scrollbarHeight ) };
      mScroll = Clamp( mScroll + scrollDY , scrollMin, scrollMax );


      if( !keyboardApi.IsPressed( Key::MouseLeft ) )
      {
        ClearActiveID();
      }
    }

    mViewportSpaceVisibleRegion.mMaxi.x -= scrollbarWidth;
  }


  void         ImGuiWindow::DrawWindowBackground()
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

  void         ImGuiWindow::EndFrame()
  {
    mDrawData->PopDebugGroup(); // pushed in ImGuiWindow::BeginFrame
  }

  void         ImGuiWindow::BeginFrame()
  {
    dynmc ImGuiGlobals& globals { ImGuiGlobals::Instance };

    const SimKeyboardApi keyboardApi { globals.mSimKeyboardApi };
    const UIStyle& style{ ImGuiGetStyle() };
    const float windowPadding{ style.windowPadding };

    mDrawData->PushDebugGroup( "BeginFrame(" + mName + ")" ); // popped in ImGuiWindow::EndFrame

    mWindowID = Hash( mName );
    mIDStack = { mWindowID };
    mMoveID = GetID( "#MOVE" );

    mViewportSpacePos = mParent ? mParent->mViewportSpaceCurrCursor : mViewportSpacePos;

    DrawWindowBackground();

    mViewportSpaceVisibleRegion = ImGuiRect::FromPosSize( mViewportSpacePos, mSize );

    Scrollbar();

    mViewportSpaceVisibleRegion.mMini += v2( 1, 1 ) * windowPadding;
    mViewportSpaceVisibleRegion.mMaxi -= v2( 1, 1 ) * windowPadding;

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

  void                          ImGuiWindow::UpdateMoveControls()
  {
    dynmc ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    const SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };
    if( keyboardApi.IsPressed( Key::MouseLeft ) )
    {
      mDesktopWindow->mRequestedPosition
        = GetMousePosViewport()
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

  void                          ImGuiWindow::BeginMoveControls()
  {
    dynmc ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    const SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };

    if( !keyboardApi.JustPressed( Key::MouseLeft ) )
      return;

    if( !IsHovered( ImGuiRect::FromPosSize( mViewportSpacePos, mSize ) ) )
      return;
      
    if( globals.mActiveID.IsValid() || globals.mHoveredID.IsValid() )
      return;

    //if( !mWindowHandleOwned )
    //  return;
    
    if( mDesktopWindow->mWindowHandle != globals.mMouseHoveredWindow )
      return;

    SetActiveID( mMoveID, this );
    globals.mActiveIDClickPos_VS = GetMousePosViewport();
    globals.mActiveIDWindowPos_SS = GetWindowPosScreenspace();
    globals.mMovingWindow = this;
  }

  void                          ImGuiWindow::ResizeControls()
  {
    //if( !mWindowHandleOwned )
    //  return;

    if( mParent )
      return;

    dynmc ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    const ImGuiID id{ GetID( "##RESIZE" ) };
    const UIStyle style{ globals.mUIStyle };
    const float windowPadding { style.windowPadding };
    const SimWindowApi windowApi{ globals.mSimWindowApi };
    const SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };
    const v2i viewportPos_SS{ windowApi.GetPos( mDesktopWindow->mWindowHandle ) };
    const v2 mousePos_VS{ GetMousePosViewport() };
    const ImGuiRect origWindowRect_VS{ ImGuiRect::FromPosSize( mViewportSpacePos, mSize ) };

    ImGuiRect targetWindowRect_VS{ origWindowRect_VS };

    const int E { 1 << 0 };
    const int S { 1 << 1 };
    const int W { 1 << 2 };
    const int N { 1 << 3 };
    int hoverMask { 0 };

    const bool anyEdgeActive{ globals.mActiveID == id };

    for( int iSide{}; iSide < 4; ++iSide )
    {
        ImGuiRect edgeRect_VS{ origWindowRect_VS };

        const int dir{ 1 << iSide };
        switch( dir )
        {
        case E: edgeRect_VS.mMini.x = edgeRect_VS.mMaxi.x - windowPadding; break;
        case S: edgeRect_VS.mMini.y = edgeRect_VS.mMaxi.y - windowPadding; break;
        case W: edgeRect_VS.mMaxi.x = edgeRect_VS.mMini.x + windowPadding; break;
        case N: edgeRect_VS.mMaxi.y = edgeRect_VS.mMini.y + windowPadding; break;
        }

        const bool hovered{ IsHovered( edgeRect_VS ) };
        const bool edgeActive{ anyEdgeActive && ( globals.mResizeMask & dir ) };

#if TAC_IMGUI_RESIZE_DEBUG()
        const bool drawBorder { true };
#else
        const bool drawBorder { hovered || edgeActive };
#endif

        if( drawBorder )
        {
          const v4 red{ 1, 0, 0, 1 };
          const v4 green{ 0, 1, 0, 1 };
          const v4 yellow{ 1, 1, 0, 1 };
          v4 color { red };

#if TAC_IMGUI_RESIZE_DEBUG()
          if( hovered )
            color = yellow;
          if( edgeActive )
            color = green;
#else
            color = ImGuiGetColor( ImGuiCol::FrameBG );
            if( edgeActive )
              color.xyz() *= 2;
            else if( hovered )
              color.xyz() *= 3;
#endif

          const UI2DDrawData::Box box
          {
            .mMini  { edgeRect_VS.mMini },
            .mMaxi  { edgeRect_VS.mMaxi},
            .mColor { color },
          };
          mDrawData->AddBox( box );
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

    if( anyEdgeActive && !keyboardApi.IsPressed( Key::MouseLeft ) )
    {
      ClearActiveID();
    }

    if( !globals.mActiveID.IsValid() &&
        hoverMask &&
        keyboardApi.JustPressed( Key::MouseLeft ) )
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

  bool         ImGuiWindow::Overlaps( const ImGuiRect& clipRect) const
  {
    return mViewportSpaceVisibleRegion.Overlaps( clipRect );
  }

  ImGuiRect    ImGuiWindow::Clip( const ImGuiRect& clipRect) const
  {
    const v2 mini{ Max( clipRect.mMini, mViewportSpaceVisibleRegion.mMini ) };
    const v2 maxi{ Min( clipRect.mMaxi, mViewportSpaceVisibleRegion.mMaxi ) };
    return ImGuiRect
    {
      .mMini { mini },
      .mMaxi { maxi },
    };
  }

  void         ImGuiWindow::ItemSize( v2 size )
  {
    mCurrLineHeight = Max( mCurrLineHeight, size.y );
    UpdateMaxCursorDrawPos( mViewportSpaceCurrCursor + v2{ size.x, mCurrLineHeight } );

    mViewportSpacePrevCursor = mViewportSpaceCurrCursor + v2( size.x, 0 );
    mPrevLineHeight = mCurrLineHeight;

    mViewportSpaceCurrCursor.x = mViewportSpacePos.x + mXOffsets.back();
    mViewportSpaceCurrCursor.y += mCurrLineHeight + ImGuiGlobals::Instance.mUIStyle.itemSpacing.y;
    mCurrLineHeight = 0;
  }

  void         ImGuiWindow::UpdateMaxCursorDrawPos( v2 pos )
  {
    mViewportSpaceMaxiCursor.x = Max( mViewportSpaceMaxiCursor.x, pos.x );
    mViewportSpaceMaxiCursor.y = Max( mViewportSpaceMaxiCursor.y, pos.y );
  }


  bool         ImGuiWindow::IsHovered( const ImGuiRect& rectViewport )
  {
    const WindowHandle mouseHoveredWindow { ImGuiGlobals::Instance.mMouseHoveredWindow };
    if( !mouseHoveredWindow.IsValid() )
      return false;

    const WindowHandle windowHandle { GetWindowHandle() };
    if( mouseHoveredWindow.GetIndex() != windowHandle.GetIndex() )
      return false;

    return rectViewport.ContainsPoint( GetMousePosViewport() );
  }

  void         ImGuiWindow::PushXOffset()
  {
    mXOffsets.push_back( mViewportSpaceCurrCursor.x - mViewportSpacePos.x );
  }

  float        ImGuiWindow::GetRemainingHeight() const
  {
    return mViewportSpaceVisibleRegion.mMaxi.y - mViewportSpaceCurrCursor.y;
  }

  float        ImGuiWindow::GetRemainingWidth() const
  {
    return mViewportSpaceVisibleRegion.mMaxi.x - mViewportSpaceCurrCursor.x;
  }

  WindowHandle ImGuiWindow::GetWindowHandle() const
  {
    return mDesktopWindow->mWindowHandle;
  }

  v2           ImGuiWindow::GetWindowPosScreenspace()
  {
    ImGuiGlobals& globals { ImGuiGlobals::Instance };
    SimWindowApi windowApi { ImGuiGlobals::Instance.mSimWindowApi };
    const v2 viewportPosScreenspace { windowApi.GetPos( mDesktopWindow->mWindowHandle ) };
    return viewportPosScreenspace + mViewportSpacePos;
  }

  v2           ImGuiWindow::GetMousePosViewport()
  {
    ImGuiGlobals& globals { ImGuiGlobals::Instance };
    SimWindowApi windowApi { globals.mSimWindowApi };
    SimKeyboardApi keyboardApi { globals.mSimKeyboardApi };
    const v2 mousePos_SS { keyboardApi.GetMousePosScreenspace() };
    const v2 windowPos_SS{ GetWindowPosScreenspace() };
    return mousePos_SS - windowPos_SS;
  }

  ImGuiID      ImGuiWindow::GetID( StringView s )
  {
    const ImGuiID id{ Hash( mIDStack.back(), Hash( s ) ) };
    return id;
  }

  void*        ImGuiWindow::GetWindowResource( ImGuiRscIdx index, ImGuiID imGuiId )
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

  ImGuiDesktopWindowImpl* ImGuiGlobals::FindDesktopWindow( WindowHandle h )
  {
    for( ImGuiDesktopWindowImpl* impl : mDesktopWindows )
      if( impl->mWindowHandle.GetIndex() == h.GetIndex() )
        return impl;

    return nullptr;
  }

  ImGuiWindow*            ImGuiGlobals::FindWindow( const StringView& name )
  {
    for( ImGuiWindow* window : mAllWindows )
      if( ( StringView )window->mName == name )
        return window;

    return nullptr;
  }

  // -----------------------------------------------------------------------------------------------

  struct CopyHelper
  {
    using GetDrawElementBytes = void* ( * )( const UI2DDrawData* );
    using GetDrawElementCount = int ( * )( const UI2DDrawData* );

    void Copy( Render::IContext* renderContext, Errors& errors ) const
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

      const int srcTotByteCount{ mSrcTotElemCount * mSizeOfElem };
      if( !srcTotByteCount )
        return;

      if( !mDst->mBuffer.IsValid() || mDst->mByteCount < srcTotByteCount )
      {
        renderDevice->DestroyBuffer( mDst->mBuffer );

        const Render::CreateBufferParams createBufferParams
        {
          .mByteCount    { srcTotByteCount },
          .mStride       { mSizeOfElem },
          .mUsage        { Render::Usage::Dynamic },
          .mBinding      { mBinding },
          .mGpuBufferFmt { mTexFmt },
          .mOptionalName { mBufName },
        };


        TAC_CALL( mDst->mBuffer =
                  renderDevice->CreateBuffer( createBufferParams, errors ) );
        mDst->mByteCount = srcTotByteCount;
      }

      const int drawCount{ mDraws->mDrawData.size() };
      Render::UpdateBufferParams* updateBufferParams{
        ( Render::UpdateBufferParams* )FrameMemoryAllocate(
          drawCount * sizeof( Render::UpdateBufferParams ) ) };

      Span< const Render::UpdateBufferParams > updates( updateBufferParams, drawCount );

      int byteOffset {};
      for( SmartPtr< UI2DDrawData >& drawData : mDraws->mDrawData )
      {
        const UI2DDrawData* pDrawData { drawData.Get() };
        const int srcByteCount { mGetDrawElementCount( pDrawData ) * mSizeOfElem };

        *updateBufferParams++ = Render::UpdateBufferParams
        {
          .mSrcBytes      { mGetDrawElementBytes( pDrawData ) },
          .mSrcByteCount  { srcByteCount },
          .mDstByteOffset { byteOffset },
        };

        byteOffset += srcByteCount;
      }

      TAC_CALL( renderContext->UpdateBuffer( mDst->mBuffer, updates, errors ) );
    }

    ImGuiRenderBuffer*    mDst;
    int                   mSizeOfElem;
    int                   mSrcTotElemCount;
    GetDrawElementBytes   mGetDrawElementBytes;
    GetDrawElementCount   mGetDrawElementCount;
    StringView            mBufName;
    ImGuiSimWindowDraws*  mDraws;
    Render::TexFmt        mTexFmt  { Render::TexFmt::kUnknown };
    Render::Binding       mBinding { Render::Binding::None };
  };

  // -----------------------------------------------------------------------------------------------

  void ImGuiSimWindowDraws::CopyIdxBuffer( Render::IContext* renderContext,
                                           ImGuiRenderBuffers* renderBuffers,
                                           Errors& errors )
  {
    auto getIdxBytes = []( const UI2DDrawData* dd ) { return ( void* )dd->mIdxs.data(); };
    auto getIdxCount = []( const UI2DDrawData* dd ) { return dd->mIdxs.size(); };

    CopyHelper idxCopyHelper
    {
      .mDst                 { &renderBuffers->mIB },
      .mSizeOfElem          { sizeof( UI2DIndex ) },
      .mSrcTotElemCount     { mIndexCount },
      .mGetDrawElementBytes { getIdxBytes },
      .mGetDrawElementCount { getIdxCount },
      .mBufName             { "imgui_idx_buf" },
      .mDraws               { this },
      .mTexFmt              { Render::TexFmt::kR16_uint },
      .mBinding             { Render::Binding::IndexBuffer },
    };

    TAC_CALL( idxCopyHelper.Copy( renderContext, errors ) );

  }

  void ImGuiSimWindowDraws::CopyVtxBuffer( Render::IContext* renderContext,
                                           ImGuiRenderBuffers* renderBuffers,
                                           Errors& errors )
  {
    auto getVtxBytes = []( const UI2DDrawData* dd ) { return ( void* )dd->mVtxs.data(); };
    auto getVtxCount = []( const UI2DDrawData* dd ) { return dd->mVtxs.size(); };
    const CopyHelper vtxCopyHelper
    {
      .mDst                 { &renderBuffers->mVB },
      .mSizeOfElem          { sizeof( UI2DVertex ) },
      .mSrcTotElemCount     { mVertexCount },
      .mGetDrawElementBytes { getVtxBytes },
      .mGetDrawElementCount { getVtxCount },
      .mBufName             { "imgui_vtx_buf" },
      .mDraws               {  this  },
      .mBinding             { Render::Binding::VertexBuffer },
    };

    TAC_CALL( vtxCopyHelper.Copy( renderContext, errors ) );

  }

  void ImGuiSimWindowDraws::CopyBuffers( Render::IContext* renderContext,
                                         ImGuiRenderBuffers* renderBuffers,
                                         Errors& errors )
  {
    CopyVtxBuffer( renderContext, renderBuffers, errors );
    CopyIdxBuffer( renderContext, renderBuffers, errors );
  }
   

  // -----------------------------------------------------------------------------------------------

  ImGuiSimWindowDraws ImGuiDesktopWindowImpl::GetSimWindowDraws()
  {
    Vector< SmartPtr< UI2DDrawData > > drawData;
    int vertexCount{};
    int indexCount{};

    const WindowHandle handle { mWindowHandle };

    // Grab all imgui windows that use this viewport window
    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
    {
      // All windows of the same viewport share the same draw data
      if( window->mDesktopWindow->mWindowHandle == mWindowHandle
          && !window->mDrawData->empty()
          && !window->mParent )
      {
        // Yoink

        window->mDrawData->mDebugGroupStack.AssertNodeHeights();

        drawData.push_back( window->mDrawData );
        vertexCount += window->mDrawData->mVtxs.size();
        indexCount += window->mDrawData->mIdxs.size();
        window->mDrawData = TAC_NEW UI2DDrawData;

        // The yoinking of the parent draw data invalidates the children draw datas
        for( ImGuiWindow* child : ImGuiGlobals::Instance.mAllWindows )
          if( child->mParent )
            child->mDrawData = child->mParent->mDrawData;
      }
    }

    return ImGuiSimWindowDraws
    {
      .mHandle      { mWindowHandle },
      .mDrawData    { drawData },
      .mVertexCount { vertexCount },
      .mIndexCount  { indexCount },
    };
  }

  // -----------------------------------------------------------------------------------------------

  void ImGuiPersistantPlatformData::UpdateAndRenderWindow( SysWindowApi windowApi,
                                                           ImGuiSimWindowDraws* simDraws,
                                                           ImGuiPersistantViewport* sysDraws,
                                                           Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const WindowHandle hDesktopWindow { sysDraws->mWindowHandle };
    if( !windowApi.IsShown( hDesktopWindow ) )
      return;

    const int n { ImGuiGlobals::Instance.mMaxGpuFrameCount };
    if( sysDraws->mRenderBuffers.size() < n )
      sysDraws->mRenderBuffers.resize( n );

    ImGuiRenderBuffers& renderBuffers { sysDraws->mRenderBuffers[ sysDraws->mFrameIndex ] };
    ( ++sysDraws->mFrameIndex ) %= n;

    TAC_CALL( Render::IContext::Scope renderContextScope{
      renderDevice->CreateRenderContext( errors ) } );
    Render::IContext* renderContext { renderContextScope.GetContext() };

    // combine draw data
    simDraws->CopyBuffers( renderContext, &renderBuffers, errors );

    const Render::SwapChainHandle fb { windowApi.GetSwapChainHandle( hDesktopWindow ) };
    const Render::SwapChainParams swapChainParams { renderDevice->GetSwapChainParams( fb ) };
    const Render::TexFmt fbFmt { swapChainParams.mColorFmt };

    const Element& element{ GetElement( fbFmt, errors ) };

    const String renderGroupStr{ String()
      + __FUNCTION__ + "(" + Tac::ToString( hDesktopWindow.GetIndex() ) + ")" };

    const v2i windowSize { windowApi.GetSize( hDesktopWindow ) };

    const Render::TextureHandle swapChainColor { renderDevice->GetSwapChainCurrentColor( fb ) };
    const Render::TextureHandle swapChainDepth { renderDevice->GetSwapChainDepth( fb ) };
    const Render::Targets renderTargets
    {
      .mColors { swapChainColor },
      .mDepth  { swapChainDepth },
    };

    UpdatePerFrame( renderContext, windowSize, errors );

    renderContext->SetVertexBuffer( renderBuffers.mVB.mBuffer );
    renderContext->SetIndexBuffer( renderBuffers.mIB.mBuffer );
    renderContext->SetPipeline( element.mPipeline );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetViewport( windowSize );
    renderContext->SetScissor( windowSize );
    renderContext->DebugEventBegin( renderGroupStr );
    renderContext->DebugMarker( "hello hello" );

    for( const SmartPtr< UI2DDrawData >& drawData : simDraws->mDrawData )
    {
      const Render::DebugGroup::Stack& debugGroupStack{ drawData->mDebugGroupStack };
      debugGroupStack.AssertNodeHeights();



      Render::DebugGroup::Iterator debugGroupIterator{
        debugGroupStack.IterateBegin( renderContext ) };

      for( const UI2DDrawCall& uidrawCall : drawData->mDrawCall2Ds )
      {
        debugGroupStack.IterateElement( debugGroupIterator, uidrawCall.mDebugGroupIndex );

        const Render::TextureHandle texture{ uidrawCall.mTexture.IsValid()
          ? uidrawCall.mTexture
          : m1x1White };

        element.mShaderImage->SetTexture( texture );


        UpdatePerObject( renderContext, uidrawCall, errors );
        renderContext->CommitShaderVariables();

        const Render::DrawArgs drawArgs
        {
          .mIndexCount { uidrawCall.mIndexCount },
          .mStartIndex { uidrawCall.mIIndexStart },
        };
        renderContext->Draw( drawArgs );
      }

      debugGroupStack.IterateEnd( debugGroupIterator );
    }

    renderContext->DebugEventEnd();
    TAC_CALL( renderContext->Execute( errors ) );
  }

  void ImGuiPersistantPlatformData::Init1x1White( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const u8 data[] { 255, 255, 255, 255 };
    const Render::Image image
    {
      .mWidth  { 1 },
      .mHeight { 1 },
      .mFormat { Render::TexFmt::kRGBA8_unorm },
    };
    const Render::CreateTextureParams::Subresource subresource
    {
      .mBytes { data },
      .mPitch { 4 },
    };
    const Render::CreateTextureParams createTextureParams
    {
      .mImage        { image },
      .mMipCount     { 1 },
      .mSubresources { &subresource },
      .mBinding      { Render::Binding::ShaderResource },
      .mOptionalName { "1x1white" },
    };
    TAC_CALL( m1x1White = renderDevice->CreateTexture( createTextureParams, errors ) );
  }

  Render::VertexDeclarations ImGuiPersistantPlatformData::GetVertexDeclarations() const
  {
    const Render::VertexDeclaration posData
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector2() },
      .mAlignedByteOffset { TAC_OFFSET_OF( UI2DVertex, mPosition ) },
    };

    const Render::VertexDeclaration uvData
    {
      .mAttribute         { Render::Attribute::Texcoord },
      .mFormat            { Render::VertexAttributeFormat::GetVector2() },
      .mAlignedByteOffset { TAC_OFFSET_OF( UI2DVertex, mGLTexCoord ) },
    };

    Render::VertexDeclarations decls;
    decls.push_back( posData );
    decls.push_back( uvData );
    return decls;
  }

  void ImGuiPersistantPlatformData::InitProgram( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams2DImage
    {
      .mFileStem { "ImGui" },
    };
    TAC_CALL( mProgram = renderDevice->CreateProgram( programParams2DImage, errors ) );
  }

  void ImGuiPersistantPlatformData::InitPerFrame( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateBufferParams perFrameParams
    {
      .mByteCount     { sizeof( PerFrameType ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "ImGui_per_frame" },
    };

    TAC_CALL( mPerFrame = renderDevice->CreateBuffer( perFrameParams, errors ) );
  }

  void ImGuiPersistantPlatformData::InitPerObject( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateBufferParams perObjectParams
    {
      .mByteCount     { sizeof( PerObjectType ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "ImGui_per_object" },
    };
    TAC_CALL( mPerObject = renderDevice->CreateBuffer( perObjectParams, errors ) );
  }

  void ImGuiPersistantPlatformData::InitSampler()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateSamplerParams samplerParams
    {
      .mFilter{ Render::Filter::Linear }
    };
    mSampler = renderDevice->CreateSampler( samplerParams );
  }

  void ImGuiPersistantPlatformData::Init( Errors& errors )
  {
    InitSampler();
    TAC_CALL( Init1x1White( errors ) );
    TAC_CALL( InitProgram( errors ) );
    TAC_CALL( InitPerFrame( errors ) );
    TAC_CALL( InitPerObject( errors ) );
  }

  Render::BlendState         ImGuiPersistantPlatformData::GetBlendState() const
  {
    // https://shawnhargreaves.com/blog/premultiplied-alpha.html
    const Render::BlendState blendState
    {
      .mSrcRGB   { Render::BlendConstants::One },
      .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
      .mBlendRGB { Render::BlendMode::Add },

      // do these 3 even matter?
      .mSrcA     { Render::BlendConstants::One },
      .mDstA     { Render::BlendConstants::Zero },
      .mBlendA   { Render::BlendMode::Add },
    };
    return blendState;
  }

  Render::DepthState         ImGuiPersistantPlatformData::GetDepthState() const
  {
    const Render::DepthState depthState
    {
      .mDepthTest  { false },
      .mDepthWrite { false },
      .mDepthFunc  { Render::DepthFunc::Less },
    };
    return depthState;
  }

  Render::RasterizerState    ImGuiPersistantPlatformData::GetRasterizerState() const
  {
    const Render::RasterizerState rasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::None },
      .mFrontCounterClockwise { true },
      .mMultisample           { false },
    };
    return rasterizerState;
  }

  ImGuiPersistantPlatformData::Element& ImGuiPersistantPlatformData::GetElement(
    Render::TexFmt texFmt,
    Errors& errors )
  {
    for( Element& element : mElements )
      if( element.mTexFmt == texFmt )
        return element;

    const Render::PipelineParams pipelineParams
    {
      .mProgram         { mProgram },
      .mBlendState      { GetBlendState() },
      .mDepthState      { GetDepthState() },
      .mRasterizerState { GetRasterizerState() },
      .mRTVColorFmts    { texFmt },
      .mVtxDecls        { GetVertexDeclarations() },
    };

    mElements.resize( mElements.size() + 1 );
    Element& element { mElements.back() };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL_RET( element, const Render::PipelineHandle pipeline{
      renderDevice->CreatePipeline( pipelineParams, errors ) } );

    Render::IShaderVar* shaderImage{
      renderDevice->GetShaderVariable( pipeline, "image" )};

    Render::IShaderVar* shaderSampler{
      renderDevice->GetShaderVariable( pipeline, "linearSampler" )};

    Render::IShaderVar* shaderPerObject{
      renderDevice->GetShaderVariable( pipeline, "perObject" )};

    Render::IShaderVar* shaderPerFrame{
      renderDevice->GetShaderVariable( pipeline, "perFrame" )};

    shaderPerFrame->SetBuffer( mPerFrame );
    shaderPerObject->SetBuffer( mPerObject );
    shaderSampler->SetSampler( mSampler );

    element = Element
    {
      .mPipeline        { pipeline },
      .mTexFmt          { texFmt },
      .mShaderImage     { shaderImage },
      .mShaderSampler   { shaderSampler },
      .mShaderPerObject { shaderPerObject },
      .mShaderPerFrame  { shaderPerFrame },
    };
    return element;
  }


    

  void ImGuiPersistantPlatformData::UpdateAndRender( ImGuiSimFrame* simFrame,
                                                     SysWindowApi windowApi,
                                                     Errors& errors )
  {
    for( ImGuiSimWindowDraws& simDraw : simFrame->mWindowDraws )
    {
      ImGuiPersistantViewport* viewportDraw { GetPersistantWindowData( simDraw.mHandle ) };
      UpdateAndRenderWindow( windowApi,
                             &simDraw,
                             viewportDraw,
                             errors );
    }
  }

  ImGuiPersistantViewport* ImGuiPersistantPlatformData::GetPersistantWindowData( WindowHandle h )
  {
    for( ImGuiPersistantViewport& viewportData : mViewportDatas )
      if( viewportData.mWindowHandle == h )
        return &viewportData;
     
    const ImGuiPersistantViewport persistantViewport
    {
      .mWindowHandle { h },
    };
    mViewportDatas.push_back( persistantViewport );
    return &mViewportDatas.back();
  }

  void ImGuiPersistantPlatformData::UpdatePerFrame( Render::IContext* context,
                                                    v2i windowSize,
                                                    Errors& errors )
  {

    const m4 proj { OrthographicUIMatrix( ( float )windowSize.x, ( float )windowSize.y ) };
    const PerFrameType perFrame
    {
      .mOrthoProj         { proj },
      .mSDFOnEdge         { FontApi::GetSDFOnEdgeValue() },
      .mSDFPixelDistScale { FontApi::GetSDFPixelDistScale() },
    };


    const Render::UpdateBufferParams updatePerFrame
    {
      .mSrcBytes     { &perFrame },
      .mSrcByteCount { sizeof( PerFrameType ) },
    };

    TAC_CALL( context->UpdateBuffer( mPerFrame, &updatePerFrame, errors ) );

  }

  void ImGuiPersistantPlatformData::UpdatePerObject( Render::IContext* context,
                                                    const UI2DDrawCall& uidrawCall,
                                                    Errors& errors )
  {

    const u32 drawType{
      [ & ]()
      {
        if( uidrawCall.mType == UI2DDrawCall::Type::kImage )
          return ( u32 )0;
        if( uidrawCall.mType == UI2DDrawCall::Type::kText )
          return ( u32 )1;
        TAC_ASSERT_INVALID_CODE_PATH;
        return ( u32 )0;
      }( )
    };

    const PerObjectType perObject
    {
      .mColor { uidrawCall.mColor },
      .mType  { drawType },
    };

    const Render::UpdateBufferParams updatePerObj
    {
      .mSrcBytes     { &perObject },
      .mSrcByteCount { sizeof( PerObjectType ) },
    };

    TAC_CALL( context->UpdateBuffer( mPerObject, &updatePerObj, errors ) );
  }



  // -----------------------------------------------------------------------------------------------

} // namespace Tac

Tac::ImGuiID Tac::GetActiveID()
  {
    return ImGuiGlobals::Instance.mActiveID;
  }

void         Tac::SetHoveredID( ImGuiID id )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  globals.mHoveredID = id;
}

void         Tac::SetActiveID( ImGuiID id, ImGuiWindow* window )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  globals.mActiveID = id;
  globals.mActiveIDWindow = window;
  globals.mActiveIDClickPos_VS = window->GetMousePosViewport();
  globals.mActiveIDWindowPos_SS = window->GetWindowPosScreenspace();
}

void         Tac::ClearActiveID()
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  globals.mActiveID = {};
  globals.mActiveIDWindow = {};
  globals.mActiveIDClickPos_VS = {};
  globals.mActiveIDWindowPos_SS = {};
}


Tac::ImGuiRscIdx Tac::ImGuiRegisterWindowResource( StringView name,
                                                  const void* initialDataBytes,
                                                  int initialDataByteCount )
{
  WindowResourceRegistry* registry = WindowResourceRegistry::GetInstance();
  return registry->RegisterResource( name,
                                     initialDataBytes,
                                     initialDataByteCount );
}
