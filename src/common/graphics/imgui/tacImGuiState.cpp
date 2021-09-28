#include "src/common/graphics/imgui/tacImGuiState.h"
#include "src/common/graphics/tacTextEdit.h"
#include "src/common/graphics/tacColorUtil.h"
#include "src/common/tacHash.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/math/tacMath.h"

#include <iostream>

namespace Tac
{
  //void ImGuiNextWindow::Clear()
  //{
  //  mSize = {};
  //}
  ImGuiNextWindow gNextWindow;

  struct RegisteredWindowResource
  {
    String         mName;
    Vector< char > mInitialData;
    ImGuiIndex     mId;
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
    RegisteredWindowResource resource;
    resource.mInitialData = initialData;
    resource.mName = name;
    resource.mId = id;
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
    //UI2DDrawData* ui2DDrawData = mDrawData;// ImGuiGlobals::Instance.mUI2DDrawData;

    const bool stuffBelowScreen = mViewportSpaceMaxiCursor.y > mViewportSpaceVisibleRegion.mMaxi.y;
    const bool stuffAboveScreen = mScroll;
    if( !stuffBelowScreen && !stuffAboveScreen )
      return;

    const float scrollbarWidth = 30;


    const v2 scrollbarBackgroundMini( mViewportSpacePos.x + mSize.x - scrollbarWidth,
                                      mViewportSpacePos.y );
    const v2 scrollbarBackgroundMaxi = mViewportSpacePos + mSize;

    //v2 mini( mPosViewport.x + mSize.x - scrollbarWidth,
    //         mPosViewport.y );
    //v2 maxi = mPosViewport + mSize;
    const v4 scrollbarBackgroundColor = v4( 0.4f, 0.2f, 0.8f, 1.0f );

    mDrawData->AddBox( scrollbarBackgroundMini,
                          scrollbarBackgroundMaxi,
                          scrollbarBackgroundColor,
                          Render::TextureHandle(),
                          nullptr );

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
        && gKeyboardInput.mMouseDeltaScroll )
      mScroll = Clamp( mScroll - gKeyboardInput.mMouseDeltaScroll * 40.0f, scrollMin, scrollMax );

    v2 scrollbarForegroundMini;
    scrollbarForegroundMini.x = 3 + scrollbarBackgroundMini.x;
    scrollbarForegroundMini.y = 3 + mViewportSpacePos.y + ( ( contentVisibleMinY - contentAllMinY ) / contentAllHeight ) * mSize.y;

    v2 scrollbarForegroundMaxi;
    scrollbarForegroundMaxi.x = -3 + scrollbarBackgroundMaxi.x;
    scrollbarForegroundMaxi.y = -3 + mViewportSpacePos.y + ( ( contentVisibleMaxY - contentAllMinY ) / contentAllHeight ) * mSize.y;

    float scrollbarHeight
      = scrollbarForegroundMaxi.y
      - scrollbarForegroundMini.y;

    v4 scrollbarForegroundColor = v4( ( scrollbarBackgroundColor.xyz() + v3( 1, 1, 1 ) ) / 2.0f, 1.0f );
    mDrawData->AddBox( scrollbarForegroundMini,
                          scrollbarForegroundMaxi,
                          scrollbarForegroundColor,
                          Render::TextureHandle(),
                          nullptr );

    static double consumeT;
    const bool hovered = IsHovered( ImGuiRect::FromMinMax( scrollbarForegroundMini,
                                                           scrollbarForegroundMaxi ) );
    if( hovered || mScrolling )
      TryConsumeMouseMovement( &consumeT, TAC_STACK_FRAME );

    if( mScrolling )
    {
      const float mouseDY
        = gKeyboardInput.mCurr.mScreenspaceCursorPos.y
        - mScrollMousePosScreenspaceInitial.y;
      mScrollMousePosScreenspaceInitial.y = gKeyboardInput.mCurr.mScreenspaceCursorPos.y;
      const float scrollDY = mouseDY * ( contentVisibleHeight / scrollbarHeight );
      mScroll = Clamp( mScroll + scrollDY , scrollMin, scrollMax );

      //std::cout
      //  << "scroll: " << mScroll << ", "
      //  << "pos y: " << gKeyboardInput.mCurr.mScreenspaceCursorPos.y << std::endl;

      if( !gKeyboardInput.IsKeyDown( Key::MouseLeft ) )
        mScrolling = false;
    }
    else if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) && hovered && consumeT )
    {
      mScrolling = true;
      mScrollMousePosScreenspaceInitial = gKeyboardInput.mCurr.mScreenspaceCursorPos;
    }

    mViewportSpaceVisibleRegion.mMaxi.x -= scrollbarWidth;
  }

  void ImGuiWindow::BeginFrame()
  {
    UI2DDrawData* ui2DDrawData = mDrawData;// ImGuiGlobals::Instance.mUI2DDrawData;

    float fhash = std::fmodf( ( ( float )HashAddString( mName ) ), 123.456f );
    //v4 childWindowColor( 0.1f, 0.15f, 0.2f, 1.0f );
    v3 childWindowColor3
      = v3( 0.1f, 0.15f, 0.2f )
      + 0.3f * GetColorSchemeA( fhash ).xyz();
    v4 childWindowColor = v4( 0.5f * childWindowColor3, 1.0f );



    if( mParent )
    {
      mViewportSpacePos = mParent->mViewportSpaceCurrCursor;

      childWindowColor.xyz() /= 2.0f;
      // Render borders
      bool clipped;
      auto clipRect = ImGuiRect::FromPosSize( mViewportSpacePos, mSize );
      ComputeClipInfo( &clipped, &clipRect );
      if( !clipped )
        ui2DDrawData->AddBox( mViewportSpacePos,
                              mViewportSpacePos + mSize,
                              childWindowColor,
                              Render::TextureHandle(),
                              nullptr );
    }
    else if( this->mStretchWindow || this->mDesktopWindowHandleOwned )
    {
      ui2DDrawData->AddBox( mViewportSpacePos,
                            mViewportSpacePos + mSize,
                            childWindowColor,
                            Render::TextureHandle(),
                            nullptr );

    }

    mViewportSpaceVisibleRegion = ImGuiRect::FromPosSize( mViewportSpacePos, mSize );
    Scrollbar();

    mViewportSpaceVisibleRegion.mMini += v2( 1, 1 ) * ImGuiGlobals::Instance.mUIStyle.windowPadding;
    mViewportSpaceVisibleRegion.mMaxi -= v2( 1, 1 ) * ImGuiGlobals::Instance.mUIStyle.windowPadding;


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
      mDesktopWindowHandle = mParent->mDesktopWindowHandle;
      mDesktopWindowHandleOwned = false;
    }
    else
    {
      if( !mIDAllocator )
        mIDAllocator = TAC_NEW ImGuiIDAllocator;
      mIDAllocator->mIDCounter = 0;
      if( // !ImGuiGlobals::Instance.mIsWindowDirectlyUnderCursor &&
          gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
      {
        mIDAllocator->mActiveID = ImGuiIdNull;
      }
    }
  }

  void ImGuiWindow::ComputeClipInfo( bool* clipped,
                                     ImGuiRect* clipRect )
  {
    //const ImGuiRect windowRect = ImGuiRect::FromPosSize( mViewportSpacePos, mSize );
    const ImGuiRect windowRect = mViewportSpaceVisibleRegion;
    *clipped =
      clipRect->mMini.x > windowRect.mMaxi.x ||
      clipRect->mMaxi.x < windowRect.mMini.x ||
      clipRect->mMini.y > windowRect.mMaxi.y ||
      clipRect->mMaxi.y < windowRect.mMini.y;
    clipRect->mMini.x = Max( clipRect->mMini.x, windowRect.mMini.x );
    clipRect->mMini.y = Max( clipRect->mMini.y, windowRect.mMini.y );
    clipRect->mMaxi.x = Min( clipRect->mMaxi.x, windowRect.mMaxi.x );
    clipRect->mMaxi.y = Min( clipRect->mMaxi.y, windowRect.mMaxi.y );
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
    if( !ImGuiGlobals::Instance.mMouseHoveredWindow.IsValid() )
      return false;
    if( ImGuiGlobals::Instance.mMouseHoveredWindow != mDesktopWindowHandle )
      return false;
    const v2 mousePosViewport = GetMousePosViewport();
    const bool result = rectViewport.ContainsPoint( mousePosViewport );
    return result;
  }

  void ImGuiWindow::PushXOffset()
  {
    mXOffsets.push_back( mViewportSpaceCurrCursor.x - mViewportSpacePos.x );
  }

  v2 ImGuiWindow::GetMousePosViewport()
  {
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    const v2 result
      = gKeyboardInput.mCurr.mScreenspaceCursorPos
      - v2( ( float )desktopWindowState->mX, ( float )desktopWindowState->mY );
    return result;
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

  ImGuiWindow* ImGuiGlobals::FindWindow( StringView name )
  {
    for( ImGuiWindow* window : mAllWindows )
      if( window->mName == name )
        return window;
    return nullptr;
  }

}
