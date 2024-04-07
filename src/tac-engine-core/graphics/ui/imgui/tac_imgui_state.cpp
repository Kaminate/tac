#include "tac_imgui_state.h" // self-inc

#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/tac_text_edit.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
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
  ImGuiPersistantPlatformData ImGuiPersistantPlatformData::Instance;
  ImGuiGlobals                ImGuiGlobals::Instance;

  // -----------------------------------------------------------------------------------------------

  struct RegisteredWindowResource
  {
    String         mName;
    Vector< char > mInitialData;
    ImGuiIndex     mId = 0;
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
    int                                mResourceCounter = 0;
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

  // -----------------------------------------------------------------------------------------------

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

  void         ImGuiWindow::Scrollbar()
  {
    ImGuiGlobals& globals = ImGuiGlobals::Instance;
    SimKeyboardApi* keyboardApi = globals.mSimKeyboardApi;

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
        && keyboardApi->GetMouseWheelDelta() )
      mScroll = Clamp( mScroll - keyboardApi->GetMouseWheelDelta() * 40.0f, scrollMin, scrollMax );

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

    //static Timestamp consumeT;
    //if( active )
    //  Mouse::TryConsumeMouseMovement( &consumeT, TAC_STACK_FRAME );

    if( mScrolling )
    {
      const float mouseDY
        = keyboardApi->GetMousePosScreenspace().y
        - mScrollMousePosScreenspaceInitial.y;
      mScrollMousePosScreenspaceInitial.y = keyboardApi->GetMousePosScreenspace().y;
      const float scrollDY = mouseDY * ( contentVisibleHeight / scrollbarHeight );
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
    const ImGuiGlobals& globals = ImGuiGlobals::Instance;
    SimKeyboardApi* keyboardApi = globals.mSimKeyboardApi;

    const UIStyle& style = ImGuiGetStyle();
    const float windowPadding = style.windowPadding;
    const bool scrollBarEnabled = globals.mScrollBarEnabled;

    mDrawData->PushDebugGroup( "ImguiWindow::BeginFrame", mName );
    TAC_ON_DESTRUCT( mDrawData->PopDebugGroup() );
    
    mViewportSpacePos = mParent ? mParent->mViewportSpaceCurrCursor : mViewportSpacePos;

    if( const bool drawWindow = mEnableBG
        && ( mParent || mStretchWindow || mWindowHandleOwned ) )
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
    return ImGuiRect
    {
      .mMini = Max( clipRect.mMini, mViewportSpaceVisibleRegion.mMini ),
      .mMaxi = Min( clipRect.mMaxi, mViewportSpaceVisibleRegion.mMaxi ),
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
    const WindowHandle mouseHoveredWindow = ImGuiGlobals::Instance.mMouseHoveredWindow;
    if( !mouseHoveredWindow.IsValid() )
      return false;

    const WindowHandle windowHandle = GetWindowHandle();
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
    ImGuiGlobals& globals = ImGuiGlobals::Instance;
    SimWindowApi* windowApi = ImGuiGlobals::Instance.mSimWindowApi;
    SimKeyboardApi* keyboardApi = globals.mSimKeyboardApi;

    const v2 mouseScreenspace = keyboardApi->GetMousePosScreenspace();
    const v2 windowScreenspace = windowApi->GetPos( mDesktopWindow->mWindowHandle );
    return mouseScreenspace - windowScreenspace;
  }

  void*        ImGuiWindow::GetWindowResource( ImGuiIndex index )
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

  void ImGuiSimWindowDraws::CopyVertexes( ImGuiRenderBuffers* gDrawInterface,
                                         Errors& errors )
  {
    if( !gDrawInterface->mVB.IsValid() || gDrawInterface->mVBCount < mVertexCount )
    {
      const int byteCount = mVertexCount * sizeof( UI2DVertex );
      gDrawInterface->mVB = TAC_CALL(
        Render::RenderApi::CreateDynBuf( byteCount, TAC_STACK_FRAME, errors ) );
      gDrawInterface->mVBCount = mVertexCount;
    }

    int byteOffset = 0;
    for( SmartPtr< UI2DDrawData>& drawData : mDrawData )
    {
      const int srcByteCount = drawData->mVtxs.size() * sizeof( UI2DVertex );
      const Render::RenderApi::UpdateDynBufParams updateParams
      {
        .mHandle = gDrawInterface->mVB,
        .mSrcBytes = drawData->mVtxs.data(),
        .mSrcByteCount = srcByteCount,
        .mDstByteOffset = byteOffset,
      };
      Render::RenderApi::UpdateDynBuf( updateParams );
      byteOffset += srcByteCount;
    }
  }

  void ImGuiSimWindowDraws::CopyIndexes( ImGuiRenderBuffers* gDrawInterface,
                                        Errors& errors )
  {
    if( !gDrawInterface->mIB.IsValid() || gDrawInterface->mIBCount < mIndexCount )
    {
      const int byteCount = mIndexCount * sizeof( UI2DIndex );
      gDrawInterface->mIB
        = TAC_CALL( Render::RenderApi::CreateDynBuf( byteCount, TAC_STACK_FRAME, errors ) );
      gDrawInterface->mIBCount = mIndexCount;
    }

    int byteOffset = 0;
    for( SmartPtr< UI2DDrawData >& drawData : mDrawData )
    {
      const int srcByteCount = drawData->mVtxs.size() * sizeof( UI2DVertex );
      const Render::RenderApi::UpdateDynBufParams updateParams
      {
        .mHandle = gDrawInterface->mIB,
        .mSrcBytes = drawData->mIdxs.data(),
        .mSrcByteCount = srcByteCount,
        .mDstByteOffset = byteOffset,
      };

      Render::RenderApi::UpdateDynBuf( updateParams );
      byteOffset += srcByteCount;
    }
  }
   
  ImGuiDesktopWindowImpl::ImGuiDesktopWindowImpl()
  {
  }

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
      .mHandle = handle,
      .mDrawData = drawData,
      .mVertexCount = vertexCount,
      .mIndexCount = indexCount,
    };
  }

  static void UpdateAndRenderWindow( ImGuiSysDrawParams* sysDrawParams,
                                     ImGuiSimWindowDraws* simDraws,
                                     ImGuiPersistantViewport* sysDraws,
                                     Errors& errors )
  {
    SysWindowApi* windowApi = sysDrawParams->mWindowApi;

    const WindowHandle hDesktopWindow = sysDraws->mWindowHandle;
    if( !windowApi->IsShown( hDesktopWindow ) )
      return;

    const int n = ImGuiGlobals::Instance.mMaxGpuFrameCount;
    if( sysDraws->mRenderBuffers.size() < n )
      sysDraws->mRenderBuffers.resize( n );

    ImGuiRenderBuffers& renderBuffers = sysDraws->mRenderBuffers[ sysDraws->mFrameIndex ];
    ( ++sysDraws->mFrameIndex ) %= n;

    // combine draw data
    simDraws->CopyVertexes( &renderBuffers, errors );
    simDraws->CopyIndexes( &renderBuffers, errors );

    //Render::ContextHandle context = TAC_CALL( Render::CreateContext( errors ) );
    void* context = nullptr;

    // TODO obviously move this to init
    const Render::ProgramParams programParams
    {
      .mFileStem = "DX12HelloFrameBuf", // test
      //.mFileStem = "2D",
    };
    Render::ProgramHandle program = TAC_CALL(
      Render::RenderApi::CreateShaderProgram( programParams, errors ) );

#if 1
    Render::FBHandle fb = windowApi->GetFBHandle( hDesktopWindow );
#endif
    const Render::TexFmt fbFmt = Render::RenderApi::GetFBFmt( fb );

    const Render::PipelineParams pipelineParams
    {
      .mProgram = program,
      .mRTVColorFmts = fbFmt,
    };
    Render::PipelineHandle pipeline = TAC_CALL(
      Render::RenderApi::CreateRenderPipeline( pipelineParams, errors ) );

#if 0

    for( ImGuiSimWindowDraws& windowDraws : frameDraws->mWindowDraws )
    {
      WindowHandle handle = windowDraws.mHandle;
      Vector< SmartPtr< UI2DDrawData > > drawData = windowDraws.mDrawData;
      const int vertexCount = windowDraws.mVertexCount;
      const int indexCount = windowDraws.mIndexCount;
    }


    TAC_RENDER_GROUP_BLOCK( String() + __FUNCTION__
                            + "("
                            + Tac::ToString( mWindowHandle.GetIndex() )
                            + ")" );
#endif

    const v2i windowSize = windowApi->GetSize( hDesktopWindow );


    const Timestamp elapsedSeconds = sysDrawParams->mTimestamp;
    const Render::DefaultCBufferPerFrame perFrameData
    {
      .mView = m4::Identity(),
      .mProjection = OrthographicUIMatrix( ( float )windowSize.x, ( float )windowSize.y ),
      .mSecModTau = ( float )Fmod( elapsedSeconds.mSeconds, 6.2831853 ),
      .mSDFOnEdge = FontApi::GetSDFOnEdgeValue(),
      .mSDFPixelDistScale = FontApi::GetSDFPixelDistScale(),
    };


#if 0
    

    TAC_ASSERT( hDesktopWindow.IsValid() );
    if(! hDesktopWindow.GetDesktopWindowNativeHandle())
      return;

    const DesktopWindowState* desktopWindowState = hDesktopWindow.GetDesktopWindowState();

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

    //TAC_CALL( Render::ExecuteCommands( { context }, errors ) );

    //mDebugGroupStack.IterateEnd( debugGroupIterator, TAC_STACK_FRAME );
    //mDebugGroupStack = {};


    for( UI2DDrawData* drawData :  frameData.mDrawData )
    {
      *drawData = {};
    }

    OS::OSDebugBreak();

#endif
  }

  void ImGuiPersistantPlatformData::UpdateAndRender( ImGuiSysDrawParams* params,
                                                     Errors& errors )
  {
    for( ImGuiSimWindowDraws& simDraw : params->mSimFrameDraws->mWindowDraws )
    {
      ImGuiPersistantViewport* viewportDraw = GetPersistantWindowData( simDraw.mHandle );
      UpdateAndRenderWindow( params,
                             &simDraw,
                             viewportDraw,
                             errors );
    }

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
