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
    ImGuiIndex     mId {};
  };

  // -----------------------------------------------------------------------------------------------

  struct WindowResourceRegistry
  {
    static WindowResourceRegistry*     GetInstance();
    ImGuiIndex                         RegisterResource( StringView name,
                                                         const void* initialDataBytes,
                                                         int initialDataByteCount );
    RegisteredWindowResource*          FindResource( ImGuiIndex index );
  private:
    Vector< RegisteredWindowResource > mRegisteredWindowResources;
    int                                mResourceCounter {};
  };

  // -----------------------------------------------------------------------------------------------

  RegisteredWindowResource* WindowResourceRegistry::FindResource( ImGuiIndex  index )
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

  ImGuiIndex                WindowResourceRegistry::RegisterResource( StringView name,
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
    const ImGuiIndex id { mResourceCounter++ };
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
    mTextInputData = TAC_NEW TextInputData;
    mDrawData = TAC_NEW UI2DDrawData;
  }

  ImGuiWindow::~ImGuiWindow()
  {
    TAC_DELETE mTextInputData;
    TAC_DELETE mDrawData;
  }

  void         ImGuiWindow::Scrollbar()
  {
    ImGuiGlobals& globals { ImGuiGlobals::Instance };
    SimKeyboardApi* keyboardApi { globals.mSimKeyboardApi };

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

    mDrawData->AddBox(bg);

    float contentAllMinY { mViewportSpacePos.y - mScroll };
    float contentAllMaxY { mViewportSpaceMaxiCursor.y };
    float contentAllHeight { contentAllMaxY - contentAllMinY };
    float contentVisibleMinY { mViewportSpacePos.y };
    float contentVisibleMaxY { mViewportSpacePos.y + mSize.y };
    float contentVisibleHeight { contentVisibleMaxY - contentVisibleMinY };

    // scrollbar min/max position
    const float scrollMin { 0 };
    const float scrollMax { contentAllHeight - contentVisibleHeight };

    // scroll with middle mouse
    if( GetActiveID() == ImGuiIdNull
        && IsHovered( ImGuiRect::FromPosSize( mViewportSpacePos, mSize ) )
        && keyboardApi->GetMouseWheelDelta() )
      mScroll = Clamp( mScroll - keyboardApi->GetMouseWheelDelta() * 40.0f, scrollMin, scrollMax );

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
    const bool hovered { IsHovered( scrollbarForegroundRect ) };
    const float scrollbarHeight { scrollbarForegroundRect.GetHeight() };
    const bool active { hovered || mScrolling };

    const UI2DDrawData::Box scrollbarBox
    {
      .mMini  { scrollbarForegroundMini },
      .mMaxi  { scrollbarForegroundMaxi },
      .mColor { ImGuiGetColor( active ? ImGuiCol::Scrollbar : ImGuiCol::ScrollbarActive ) },
    };

    mDrawData->AddBox( scrollbarBox );

    //static Timestamp consumeT;
    //if( active )
    //  Mouse::TryConsumeMouseMovement( &consumeT, TAC_STACK_FRAME );

    if( mScrolling )
    {
      const float mouseDY{
        keyboardApi->GetMousePosScreenspace().y
       - mScrollMousePosScreenspaceInitial.y };
      mScrollMousePosScreenspaceInitial.y = keyboardApi->GetMousePosScreenspace().y;
      const float scrollDY{ mouseDY * ( contentVisibleHeight / scrollbarHeight ) };
      mScroll = Clamp( mScroll + scrollDY , scrollMin, scrollMax );


      if( !keyboardApi->IsPressed( Key::MouseLeft ) )
        mScrolling = false;
    }
    else if( keyboardApi->JustPressed( Key::MouseLeft ) && hovered )//&& consumeT )
    {
      mScrolling = true;
      mScrollMousePosScreenspaceInitial = keyboardApi->GetMousePosScreenspace();
    }

    mViewportSpaceVisibleRegion.mMaxi.x -= scrollbarWidth;
  }

  void         ImGuiWindow::BeginFrame()
  {
    const ImGuiGlobals& globals { ImGuiGlobals::Instance };
    SimKeyboardApi* keyboardApi { globals.mSimKeyboardApi };

    const UIStyle& style { ImGuiGetStyle() };
    const float windowPadding { style.windowPadding };
    const bool scrollBarEnabled { globals.mScrollBarEnabled };

    mDrawData->PushDebugGroup( "ImguiWindow::BeginFrame", mName );
    TAC_ON_DESTRUCT( mDrawData->PopDebugGroup() );
    
    mViewportSpacePos = mParent ? mParent->mViewportSpaceCurrCursor : mViewportSpacePos;

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

    mViewportSpaceVisibleRegion = ImGuiRect::FromPosSize( mViewportSpacePos, mSize );

    if( scrollBarEnabled )
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
      mIDAllocator = mParent->mIDAllocator;
      mDesktopWindow = mParent->mDesktopWindow;
      mWindowHandleOwned = false;
    }
    else
    {
      if( !mIDAllocator )
        mIDAllocator = TAC_NEW ImGuiIDAllocator;

      mIDAllocator->mIDCounter = 0;
      if( keyboardApi->JustPressed( Key::MouseLeft ) )
      {
        mIDAllocator->mActiveID = ImGuiIdNull;
      }
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

  ImGuiId      ImGuiWindow::GetActiveID()
  {
    return mIDAllocator->mActiveID;
  }

  void         ImGuiWindow::SetActiveID( ImGuiId id )
  {
    mIDAllocator->mActiveID = id;
  }

  ImGuiId      ImGuiWindow::GetID()
  {
    return mIDAllocator->mIDCounter++;
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

  float        ImGuiWindow::GetRemainingWidth() const
  {
    return mViewportSpaceVisibleRegion.mMaxi.x - mViewportSpaceCurrCursor.x;
  }

  WindowHandle ImGuiWindow::GetWindowHandle() const
  {
    return mDesktopWindow->mWindowHandle;
  }

  v2           ImGuiWindow::GetMousePosViewport()
  {
    ImGuiGlobals& globals { ImGuiGlobals::Instance };
    SimWindowApi* windowApi { ImGuiGlobals::Instance.mSimWindowApi };
    SimKeyboardApi* keyboardApi { globals.mSimKeyboardApi };
    const v2 mouseScreenspace { keyboardApi->GetMousePosScreenspace() };
    const v2 windowScreenspace { windowApi->GetPos( mDesktopWindow->mWindowHandle ) };
    return mouseScreenspace - windowScreenspace;
  }

  void*        ImGuiWindow::GetWindowResource( ImGuiIndex index )
  {
    ImGuiId imGuiId { GetID() };
    for( ImGuiWindowResource& resource : mResources )
      if( resource.mImGuiId == imGuiId && resource.mIndex == index )
        return resource.mData.data();

    WindowResourceRegistry* registry{ WindowResourceRegistry::GetInstance() };
    RegisteredWindowResource* pRegistered { registry->FindResource( index ) };
    TAC_ASSERT( pRegistered );

    mResources.resize( mResources.size() + 1 );
    ImGuiWindowResource& resource { mResources.back() };
    resource = ImGuiWindowResource
    {
      .mImGuiId { imGuiId },
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
    Render::TexFmt        mTexFmt { Render::TexFmt::kUnknown };
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
      .mDraws               {  this  },
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
    WindowHandle handle = mWindowHandle;

    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
    {
      if( window->mDesktopWindow != this )
        continue;

      if( window->mDrawData->empty() )
        continue;

      // Yoink!
      drawData.push_back( window->mDrawData );
      vertexCount += window->mDrawData->mVtxs.size();
      indexCount += window->mDrawData->mIdxs.size();
      window->mDrawData = TAC_NEW UI2DDrawData;
    }

    return ImGuiSimWindowDraws
    {
      .mHandle      { handle },
      .mDrawData    { drawData },
      .mVertexCount { vertexCount },
      .mIndexCount  { indexCount },
    };
  }

  // -----------------------------------------------------------------------------------------------

  void ImGuiPersistantPlatformData::UpdateAndRenderWindow( ImGuiSysDrawParams sysDrawParams,
                                                           ImGuiSimWindowDraws* simDraws,
                                                           ImGuiPersistantViewport* sysDraws,
                                                           Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const SysWindowApi* windowApi { sysDrawParams.mWindowApi };

    const WindowHandle hDesktopWindow { sysDraws->mWindowHandle };
    if( !windowApi->IsShown( hDesktopWindow ) )
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

    const Render::SwapChainHandle fb { windowApi->GetSwapChainHandle( hDesktopWindow ) };
    const Render::SwapChainParams swapChainParams { renderDevice->GetSwapChainParams( fb ) };
    const Render::TexFmt fbFmt { swapChainParams.mColorFmt };

    const Element& element{ GetElement( fbFmt, errors ) };

    const String renderGroupStr{ String()
      + __FUNCTION__ + "(" + Tac::ToString( hDesktopWindow.GetIndex() ) + ")" };

    const v2i windowSize { windowApi->GetSize( hDesktopWindow ) };

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


    //Render::DebugGroup::Iterator debugGroupIterator = mDebugGroupStack.IterateBegin();

    for( SmartPtr< UI2DDrawData >& drawData : simDraws->mDrawData )
    {
      for( const UI2DDrawCall& uidrawCall : drawData->mDrawCall2Ds )
      {
        //mDebugGroupStack.IterateElement( debugGroupIterator,
        //                                 uidrawCall.mDebugGroupIndex,
        //                                 uidrawCall.mStackFrame );

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
    }

    renderContext->DebugEventEnd();
    TAC_CALL( renderContext->Execute( errors ) );

    //mDebugGroupStack.IterateEnd( debugGroupIterator, TAC_STACK_FRAME );
    //mDebugGroupStack = {};
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
    mSampler = renderDevice->CreateSampler( Render::Filter::Linear );
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
    const Render::BlendState blendState
    {
      .mSrcRGB   { Render::BlendConstants::One },
      .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::One },
      .mDstA     { Render::BlendConstants::OneMinusSrcA },
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


    

  void ImGuiPersistantPlatformData::UpdateAndRender( ImGuiSysDrawParams params,
                                                     Errors& errors )
  {
    for( ImGuiSimWindowDraws& simDraw : params.mSimFrameDraws->mWindowDraws )
    {
      ImGuiPersistantViewport* viewportDraw { GetPersistantWindowData( simDraw.mHandle ) };
      UpdateAndRenderWindow( params,
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

Tac::ImGuiIndex Tac::ImGuiRegisterWindowResource( StringView name,
                                                  const void* initialDataBytes,
                                                  int initialDataByteCount )
{
  WindowResourceRegistry* registry = WindowResourceRegistry::GetInstance();
  return registry->RegisterResource( name,
                                     initialDataBytes,
                                     initialDataByteCount );
}
