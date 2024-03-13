#include "tac_imgui_state.h" // self-inc

#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/color/tac_color_util.h"
#include "tac-rhi/ui/tac_text_edit.h"
#include "tac-rhi/ui/tac_ui_2d.h"
#include "tac-rhi/ui/tac_ui_2d.h"
#include "tac-rhi/render/tac_render.h" // CreateContext
#include "tac-std-lib/input/tac_keyboard_input.h"
#include "tac-rhi/ui/tac_font.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/os/tac_os.h"

// This is a shell include, how is it in tac/src/common?
#include "src/shell/tac_desktop_window_graphics.h"

namespace Tac
{
  ImGuiNextWindow gNextWindow;

  struct RegisteredWindowResource
  {
    String         mName;
    Vector< char > mInitialData;
    ImGuiIndex     mId = 0;
  };

  struct WindowResourceRegistry
  {
    static WindowResourceRegistry*     GetInstance();
    ImGuiIndex                         RegisterResource( StringView name,
                                                         const void* initialDataBytes,
                                                         int initialDataByteCount );
    RegisteredWindowResource*          FindResource( ImGuiIndex index );
  private:
    Vector< RegisteredWindowResource > mRegisteredWindowResources;
    int                                mResourceCounter = 0;
  };

  RegisteredWindowResource* WindowResourceRegistry::FindResource( ImGuiIndex  index )
  {
    for( RegisteredWindowResource& resource : mRegisteredWindowResources )
      if( resource.mId == index )
        return &resource;
    return nullptr;
  }

  WindowResourceRegistry* WindowResourceRegistry::GetInstance()
  {
    static WindowResourceRegistry instance;
    return &instance;
  }

  ImGuiIndex WindowResourceRegistry::RegisterResource( StringView name,
                                                       const void* initialDataBytes,
                                                       int initialDataByteCount )
  {
    Vector< char > initialData;
    if( initialDataBytes )
    {
      const char* dataBegin = ( char* )initialDataBytes;
      const char* dataEnd = ( char* )initialDataBytes + initialDataByteCount;
      initialData.assign( dataBegin, dataEnd );
    }
    else
    {
      initialData.assign( initialDataByteCount, 0 );
    }
    const ImGuiIndex id = mResourceCounter++;
    const RegisteredWindowResource resource{ .mName = name,
                                             .mInitialData = initialData,
                                             .mId = id};
    mRegisteredWindowResources.push_back( resource );
    return id;
  }

  ImGuiIndex ImGuiRegisterWindowResource( StringView name,
                                          const void* initialDataBytes,
                                          int initialDataByteCount )
  {
    return WindowResourceRegistry::GetInstance()->RegisterResource( name,
                                                                    initialDataBytes,
                                                                    initialDataByteCount );

  }

  ImGuiGlobals ImGuiGlobals::Instance;

  ImGuiWindow::ImGuiWindow()
  {
    mTextInputData = TAC_NEW TextInputData;
    mDrawData = TAC_NEW UI2DDrawData;
  }

  ImGuiWindow::~ImGuiWindow()
  {
    delete mTextInputData;
    delete mDrawData;
  }

  void ImGuiWindow::Scrollbar()
  {
    const bool stuffBelowScreen = mViewportSpaceMaxiCursor.y > mViewportSpaceVisibleRegion.mMaxi.y;
    const bool stuffAboveScreen = mScroll;
    if( !stuffBelowScreen && !stuffAboveScreen )
      return;

    mDrawData->PushDebugGroup( "scrollbar" );
    TAC_ON_DESTRUCT( mDrawData->PopDebugGroup() );

    const float scrollbarWidth = 30;


    const v2 scrollbarBackgroundMini = mViewportSpacePos + v2( mSize.x - scrollbarWidth, 0 );
    const v2 scrollbarBackgroundMaxi = mViewportSpacePos + mSize;

    const UI2DDrawData::Box bg =
    {
      .mMini = scrollbarBackgroundMini,
      .mMaxi = scrollbarBackgroundMaxi,
      .mColor = ImGuiGetColor( ImGuiCol::ScrollbarBG ),
    };

    mDrawData->AddBox(bg);

    float contentAllMinY = mViewportSpacePos.y - mScroll;
    float contentAllMaxY = mViewportSpaceMaxiCursor.y;
    float contentAllHeight = contentAllMaxY - contentAllMinY;
    float contentVisibleMinY = mViewportSpacePos.y;
    float contentVisibleMaxY = mViewportSpacePos.y + mSize.y;
    float contentVisibleHeight = contentVisibleMaxY - contentVisibleMinY;

    // scrollbar min/max position
    const float scrollMin = 0;
    const float scrollMax = contentAllHeight - contentVisibleHeight;

    // scroll with middle mouse
    if( GetActiveID() == ImGuiIdNull
        && IsHovered( ImGuiRect::FromPosSize( mViewportSpacePos, mSize ) )
        && Mouse::GetMouseDeltaScroll() )
      mScroll = Clamp( mScroll - Mouse::GetMouseDeltaScroll() * 40.0f, scrollMin, scrollMax );

    const v2 scrollbarForegroundMini
    {
       3 + scrollbarBackgroundMini.x,
       3 + mViewportSpacePos.y + ( ( contentVisibleMinY - contentAllMinY ) / contentAllHeight ) * mSize.y,
    };

    const v2 scrollbarForegroundMaxi
    {
       -3 + scrollbarBackgroundMaxi.x,
       -3 + mViewportSpacePos.y + ( ( contentVisibleMaxY - contentAllMinY ) / contentAllHeight ) * mSize.y,
    };

    const ImGuiRect scrollbarForegroundRect = ImGuiRect::FromMinMax( scrollbarForegroundMini,
                                                                     scrollbarForegroundMaxi );
    const bool hovered = IsHovered( scrollbarForegroundRect );
    const float scrollbarHeight = scrollbarForegroundRect.GetHeight();
    const bool active = hovered || mScrolling;

    const UI2DDrawData::Box scrollbarBox =
    {
      .mMini = scrollbarForegroundMini,
      .mMaxi = scrollbarForegroundMaxi,
      .mColor =ImGuiGetColor( active ? ImGuiCol::Scrollbar : ImGuiCol::ScrollbarActive ), 
    };

    mDrawData->AddBox( scrollbarBox );

    static Timestamp consumeT;
    if( active )
      Mouse::TryConsumeMouseMovement( &consumeT, TAC_STACK_FRAME );

    if( mScrolling )
    {
      const float mouseDY
        = Mouse::GetScreenspaceCursorPos().y
        - mScrollMousePosScreenspaceInitial.y;
      mScrollMousePosScreenspaceInitial.y = Mouse::GetScreenspaceCursorPos().y;
      const float scrollDY = mouseDY * ( contentVisibleHeight / scrollbarHeight );
      mScroll = Clamp( mScroll + scrollDY , scrollMin, scrollMax );


      if( !Mouse::ButtonIsDown( Mouse::Button::MouseLeft ) )
        mScrolling = false;
    }
    else if( Mouse::ButtonJustDown( Mouse::Button::MouseLeft ) && hovered && consumeT )
    {
      mScrolling = true;
      mScrollMousePosScreenspaceInitial = Mouse::GetScreenspaceCursorPos();
    }

    mViewportSpaceVisibleRegion.mMaxi.x -= scrollbarWidth;
  }

  void ImGuiWindow::BeginFrame()
  {
    const ImGuiGlobals& globals = ImGuiGlobals::Instance;
    const UIStyle& style = ImGuiGetStyle();
    const float windowPadding = style.windowPadding;
    const bool scrollBarEnabled = globals.mScrollBarEnabled;

    mDrawData->PushDebugGroup( "ImguiWindow::BeginFrame", mName );
    TAC_ON_DESTRUCT( mDrawData->PopDebugGroup() );
    
    mViewportSpacePos = mParent ? mParent->mViewportSpaceCurrCursor : mViewportSpacePos;

    if( const bool drawWindow = mEnableBG
        && ( mParent || mStretchWindow || mDesktopWindowHandleOwned ) )
    {
      const ImGuiRect origRect = ImGuiRect::FromPosSize( mViewportSpacePos, mSize );
      if( Overlaps( origRect ) )
      {
        const ImGuiRect clipRect = Clip( origRect );
        const ImGuiCol col = mParent ? ImGuiCol::ChildWindowBackground : ImGuiCol::WindowBackground;
        const UI2DDrawData::Box box =
        {
          .mMini = clipRect.mMini,
          .mMaxi = clipRect.mMaxi,
          .mColor = ImGuiGetColor(col),
        };
        mDrawData->AddBox( box, &clipRect );
      }
    }

    mViewportSpaceVisibleRegion = ImGuiRect::FromPosSize( mViewportSpacePos, mSize );

    if( scrollBarEnabled )
      Scrollbar();

    mViewportSpaceVisibleRegion.mMini += v2( 1, 1 ) * windowPadding;
    mViewportSpaceVisibleRegion.mMaxi -= v2( 1, 1 ) * windowPadding;

    const v2 drawPos = mViewportSpaceVisibleRegion.mMini - v2( 0, mScroll );
    
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
      mDesktopWindowHandleOwned = false;
    }
    else
    {
      if( !mIDAllocator )
        mIDAllocator = TAC_NEW ImGuiIDAllocator;

      mIDAllocator->mIDCounter = 0;
      if( Mouse::ButtonJustDown( Mouse::Button::MouseLeft ) )
      {
        mIDAllocator->mActiveID = ImGuiIdNull;
      }
    }
  }

  bool  ImGuiWindow::Overlaps( const ImGuiRect& clipRect) const
  {
    return mViewportSpaceVisibleRegion.Overlaps( clipRect );
  }

  ImGuiRect  ImGuiWindow::Clip( const ImGuiRect& clipRect) const
  {
    return ImGuiRect
    {
      .mMini = Max( clipRect.mMini, mViewportSpaceVisibleRegion.mMini ),
      .mMaxi = Min( clipRect.mMaxi, mViewportSpaceVisibleRegion.mMaxi ),
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

  ImGuiId ImGuiWindow::GetActiveID()
  {
    return mIDAllocator->mActiveID;
  }

  void ImGuiWindow::SetActiveID( ImGuiId id )
  {
    mIDAllocator->mActiveID = id;
  }

  ImGuiId ImGuiWindow::GetID()
  {
    return mIDAllocator->mIDCounter++;
  }

  bool ImGuiWindow::IsHovered( const ImGuiRect& rectViewport )
  {
    const DesktopWindowHandle mouseHoveredWindow = ImGuiGlobals::Instance.mMouseHoveredWindow;
    const DesktopWindowHandle desktopWindowHandle = GetDesktopWindowHandle();
    return
      mouseHoveredWindow.IsValid() &&
      desktopWindowHandle.IsValid() &&
      mouseHoveredWindow == desktopWindowHandle &&
      rectViewport.ContainsPoint( GetMousePosViewport() );
  }

  void ImGuiWindow::PushXOffset()
  {
    mXOffsets.push_back( mViewportSpaceCurrCursor.x - mViewportSpacePos.x );
  }

  float ImGuiWindow::GetRemainingWidth() const
  {
    return mViewportSpaceVisibleRegion.mMaxi.x - mViewportSpaceCurrCursor.x;
  }

  DesktopWindowHandle           ImGuiWindow::GetDesktopWindowHandle() const
  {
    return mDesktopWindow->mDesktopWindowHandle;
  }

  const DesktopWindowState* ImGuiWindow::GetDesktopWindowState() const
  {
    DesktopWindowHandle desktopWindowHandle = GetDesktopWindowHandle();
    return Tac::GetDesktopWindowState( desktopWindowHandle );
  }

  v2 ImGuiWindow::GetMousePosViewport()
  {
    return Mouse::GetScreenspaceCursorPos() - GetDesktopWindowState()->GetPosV2();
  }

  void* ImGuiWindow::GetWindowResource( ImGuiIndex index )
  {
    ImGuiId imGuiId = GetID();
    for( ImGuiWindowResource& resource : mResources )
      if( resource.mImGuiId == imGuiId && resource.mIndex == index )
        return resource.mData.data();

    RegisteredWindowResource* pRegistered = WindowResourceRegistry::GetInstance()->FindResource( index );
    TAC_ASSERT( pRegistered );

    mResources.resize( mResources.size() + 1 );
    ImGuiWindowResource& resource = mResources.back();
    resource.mData = pRegistered->mInitialData;
    resource.mImGuiId = imGuiId;
    resource.mIndex = index;
    return resource.mData.data();
  }

  ImGuiDesktopWindowImpl* ImGuiGlobals::FindDesktopWindow( DesktopWindowHandle h )
  {
    for( ImGuiDesktopWindowImpl* impl : mDesktopWindows )
      if( impl->mDesktopWindowHandle == h )
        return impl;

    return nullptr;
  }

  ImGuiWindow* ImGuiGlobals::FindWindow( const StringView& name )
  {
    for( ImGuiWindow* window : mAllWindows )
      if( window->mName == name )
        return window;

    return nullptr;
  }

  // -----------------------------------------------------------------------------------------------

  void ImGuiDesktopWindowImpl::FrameDrawData::CopyVertexes( Render::ContextHandle context,
                                                            ImGuiRenderBuffers* gDrawInterface,
                                                            Errors& errors )
  {

    if( !gDrawInterface->mVB.IsValid() || gDrawInterface->mVBCount < mVertexCount )
    {
      const int byteCount = mVertexCount * sizeof( UI2DVertex );
      gDrawInterface->mVB = context.CreateDynamicBuffer( byteCount, TAC_STACK_FRAME );
      gDrawInterface->mVBCount = mVertexCount;
    }

    int byteOffset = 0;
    for( UI2DDrawData* drawData : mDrawData )
    {
      struct UpdateVtx : public Render::UpdateMemory
      {
        UpdateVtx( Vector< UI2DVertex >& vtxs )   { mVtxs.swap( vtxs ); }
        const void* GetBytes() const override     { return mVtxs.data(); }
        int         GetByteCount() const override { return mVtxs.size() * sizeof( UI2DVertex ); }
        Vector< UI2DVertex > mVtxs;
      }* src = TAC_NEW UpdateVtx{ drawData->mVtxs };

      context.UpdateDynamicBuffer( gDrawInterface->mVB, byteOffset, src );
      byteOffset += src->GetByteCount();
    }
  }

  void ImGuiDesktopWindowImpl::FrameDrawData::CopyIndexes( Render::ContextHandle context,
                                                           ImGuiRenderBuffers* gDrawInterface,
                                                           Errors& errors )
  {
    if( !gDrawInterface->mIB.IsValid() || gDrawInterface->mIBCount < mIndexCount )
    {
      const int byteCount = mIndexCount * sizeof( UI2DIndex );
      gDrawInterface->mIB = context.CreateDynamicBuffer( byteCount, TAC_STACK_FRAME );
      gDrawInterface->mIBCount = mIndexCount;
    }

    int byteOffset = 0;
    for( UI2DDrawData* drawData : mDrawData )
    {
      struct UpdateIdx : public Render::UpdateMemory
      {
        UpdateIdx( Vector< UI2DIndex >& idxs )    { mIdxs.swap( idxs ); }
        const void* GetBytes() const override     { return mIdxs.data(); }
        int         GetByteCount() const override { return mIdxs.size() * sizeof( UI2DIndex ); }
        Vector< UI2DIndex > mIdxs;
      }* src = TAC_NEW UpdateIdx{ drawData->mIdxs };

      context.UpdateDynamicBuffer( gDrawInterface->mIB, byteOffset, src );
      byteOffset += src->GetByteCount();
    }
  }
   
  ImGuiDesktopWindowImpl::ImGuiDesktopWindowImpl()
    : mRenderBuffers( ImGuiGlobals::Instance.mMaxGpuFrameCount )
  {
  }


  ImGuiDesktopWindowImpl::FrameDrawData ImGuiDesktopWindowImpl::GetFrameData()
  {
    Vector< UI2DDrawData* > mDrawData;
    int mVertexCount{};
    int mIndexCount{};

    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
    {
      if( window->mDesktopWindow != this || Contains( mDrawData, window->mDrawData ) )
        continue;

      mDrawData.push_back( window->mDrawData );
      mVertexCount += window->mDrawData->mVtxs.size();
      mIndexCount += window->mDrawData->mIdxs.size();
    }

    return FrameDrawData
    {
      .mDrawData = mDrawData,
      .mVertexCount = mVertexCount,
      .mIndexCount = mIndexCount,
    };
  }

  void ImGuiDesktopWindowImpl::Render( Errors& errors )
  {
    if( !mDesktopWindowHandle.IsValid() )
      return;

    FrameDrawData frameData = GetFrameData();

    if( frameData.mVertexCount == 0 || frameData.mIndexCount == 0 )
      return;

    ImGuiRenderBuffers& renderBuffers = mRenderBuffers[ mFrameIndex ];
    ( ++mFrameIndex ) %= ImGuiGlobals::Instance.mMaxGpuFrameCount;

    // combine draw data
    Render::ContextHandle context = TAC_CALL( Render::CreateContext( errors ) );
    TAC_CALL( frameData.CopyVertexes( context, &renderBuffers, errors ) );
    TAC_CALL( frameData.CopyIndexes( context, &renderBuffers, errors ) );

#if 0
    TAC_RENDER_GROUP_BLOCK( String() + __FUNCTION__
                            + "("
                            + Tac::ToString( mDesktopWindowHandle.GetIndex() )
                            + ")" );
#endif

    const DesktopWindowHandle hDesktopWindow = mDesktopWindowHandle;

    TAC_ASSERT( hDesktopWindow.IsValid() );
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( hDesktopWindow );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    const v2 size = desktopWindowState->GetSizeV2();
    const int w = desktopWindowState->mWidth;
    const int h = desktopWindowState->mHeight;

    WindowGraphics& windowGraphics = WindowGraphics::Instance();

    //Render::ViewHandle2 viewHandle2 = ;
    OS::OSDebugBreak();
    const Render::ViewHandle viewHandle = windowGraphics.GetView( hDesktopWindow );
    if( !viewHandle.IsValid() )
      return;

    const Render::FramebufferHandle hFB = windowGraphics.GetFramebuffer( hDesktopWindow );

    Render::SetViewFramebuffer( viewHandle, hFB );
    Render::SetViewport( viewHandle, Render::Viewport( size ) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect( size ) );

    //Render::DebugGroup::Iterator debugGroupIterator = mDebugGroupStack.IterateBegin();

    const Timestamp elapsedSeconds = Timestep::GetElapsedTime();
    const Render::DefaultCBufferPerFrame perFrameData
    {
      .mView = m4::Identity(),
      .mProjection = OrthographicUIMatrix( ( float )w, ( float )h ),
      .mSecModTau = ( float )Fmod( elapsedSeconds.mSeconds, 6.2831853 ),
      .mSDFOnEdge = FontApi::GetSDFOnEdgeValue(),
      .mSDFPixelDistScale = FontApi::GetSDFPixelDistScale(),
    };

    const Render::ConstantBufferHandle hPerFrame = Render::DefaultCBufferPerFrame::Handle;
    const int perFrameSize = sizeof( Render::DefaultCBufferPerFrame );
    Render::UpdateConstantBuffer( hPerFrame, &perFrameData, perFrameSize, TAC_STACK_FRAME );


    for( UI2DDrawData* drawData : frameData.mDrawData )
    {
      for( const UI2DDrawCall& uidrawCall : drawData->mDrawCall2Ds )
      {
        //mDebugGroupStack.IterateElement( debugGroupIterator,
        //                                 uidrawCall.mDebugGroupIndex,
        //                                 uidrawCall.mStackFrame );

        //const Render::TextureHandle texture = uidrawCall.mTexture.IsValid() ?
        //  uidrawCall.mTexture :
        //  gUI2DCommonData.m1x1White;


        const Render::ConstantBufferHandle hPerObj = Render::DefaultCBufferPerObject::Handle;
        const int perObjSize = sizeof( Render::DefaultCBufferPerObject );
        //Render::UpdateConstantBuffer( hPerObj,
        //                              &uidrawCall.mUniformSource,
        //                              perObjSize,
        //                              uidrawCall.mStackFrame );
        //Render::SetBlendState( gUI2DCommonData.mBlendState );
        //Render::SetRasterizerState( gUI2DCommonData.mRasterizerState );
        //Render::SetSamplerState( { gUI2DCommonData.mSamplerState } );
        //Render::SetDepthState( gUI2DCommonData.mDepthState );
        //Render::SetVertexFormat( gUI2DCommonData.mFormat );
        //Render::SetVertexBuffer( mVertexBufferHandle, uidrawCall.mIVertexStart, uidrawCall.mVertexCount );
        //Render::SetIndexBuffer( mIndexBufferHandle, uidrawCall.mIIndexStart, uidrawCall.mIndexCount );
        //Render::SetTexture( { texture } );
        //Render::SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
        //Render::SetShader( uidrawCall.mShader );
        //Render::Submit( viewHandle, uidrawCall.mStackFrame );
      }
    }

    TAC_CALL( Render::ExecuteCommands( { context }, errors ) );

    //mDebugGroupStack.IterateEnd( debugGroupIterator, TAC_STACK_FRAME );
    //mDebugGroupStack = {};


    for( UI2DDrawData* drawData :  frameData.mDrawData )
    {
      *drawData = {};
    }

    OS::OSDebugBreak();

  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac
