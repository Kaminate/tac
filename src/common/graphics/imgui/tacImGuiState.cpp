#include "src/common/graphics/imgui/tacImGuiState.h"
#include "src/common/graphics/tacTextEdit.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/math/tacMath.h"

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
    ImGuiindex     mId;
  };

  struct WindowResourceRegistry
  {
    static WindowResourceRegistry*     GetInstance();
    ImGuiindex                         RegisterResource( StringView name,
                                                         void* initialDataBytes,
                                                         int initialDataByteCount );
    RegisteredWindowResource*          FindResource( ImGuiindex index );
  private:
    Vector< RegisteredWindowResource > mRegisteredWindowResources;
    int                                mResourceCounter = 0;
  };

  RegisteredWindowResource* WindowResourceRegistry::FindResource( ImGuiindex  index )
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

  ImGuiindex WindowResourceRegistry::RegisterResource( StringView name,
                                                       void* initialDataBytes,
                                                       int initialDataByteCount )
  {
    const char* dataBegin = ( char* )initialDataBytes;
    const char* dataEnd = ( char* )initialDataBytes + initialDataByteCount;
    const ImGuiindex id = mResourceCounter++;
    RegisteredWindowResource resource;
    resource.mInitialData.assign( dataBegin, dataEnd );
    resource.mName = name;
    resource.mId = id;
    mRegisteredWindowResources.push_back( resource );
    return id;
  }

  ImGuiindex ImGuiRegisterWindowResource( StringView name,
                                          void* initialDataBytes,
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

  void ImGuiWindow::BeginFrame()
  {
    UI2DDrawData* ui2DDrawData = mDrawData;// ImGuiGlobals::Instance.mUI2DDrawData;

    if( mParent )
    {
      mPosViewport = mParent->mCurrCursorViewport;
      // Render borders
      {
        bool clipped;
        auto clipRect = ImGuiRect::FromPosSize( mPosViewport, mSize );
        ComputeClipInfo( &clipped, &clipRect );
        const v4 childWindowColor( 0.1f, 0.15f, 0.2f, 1.0f );
        if( !clipped )
          ui2DDrawData->AddBox( mPosViewport,
                                mPosViewport + mSize,
                                childWindowColor,
                                Render::TextureHandle(),
                                nullptr );
      }
    }

    // Scrollbar
    if( mMaxiCursorViewport.y > mPosViewport.y + mSize.y || mScroll )
    {
      const float scrollbarWidth = 30;
      v2 mini( mPosViewport.x + mSize.x - scrollbarWidth,
               mPosViewport.y );
      v2 maxi = mPosViewport + mSize;
      v4 scrollbarBackgroundColor = v4( 0.4f, 0.2f, 0.8f, 1.0f );
      Render::TextureHandle invalidTexture;

      ui2DDrawData->AddBox( mini, maxi, scrollbarBackgroundColor, invalidTexture, nullptr );

      float contentAllMinY = mPosViewport.y - mScroll;
      float contentAllMaxY = mMaxiCursorViewport.y;
      float contentAllHeight = contentAllMaxY - contentAllMinY;
      float contentVisibleMinY = mPosViewport.y;
      float contentVisibleMaxY = mPosViewport.y + mSize.y;
      float contentVisibleHeight = contentVisibleMaxY - contentVisibleMinY;

      mini.y = mPosViewport.y + ( ( contentVisibleMinY - contentAllMinY ) / contentAllHeight ) * mSize.y;
      maxi.y = mPosViewport.y + ( ( contentVisibleMaxY - contentAllMinY ) / contentAllHeight ) * mSize.y;

      v2 padding = v2( 1, 1 ) * 3;
      mini += padding;
      maxi -= padding;

      v4 scrollbarForegroundColor = v4( ( scrollbarBackgroundColor.xyz() + v3( 1, 1, 1 ) ) / 2.0f, 1.0f );
      ui2DDrawData->AddBox( mini, maxi, scrollbarForegroundColor, invalidTexture, nullptr );


      if( mScrolling )
      {
        Errors mouseErrors;
        v2 mousePosScreenspace;
        OS::GetScreenspaceCursorPos( mousePosScreenspace, mouseErrors );
        if( mouseErrors.empty() )
        {
          float mouseDY = mousePosScreenspace.y - mScrollMousePosScreenspaceInitial.y;
          float scrollMin = 0;
          float scrollMax = contentAllHeight - contentVisibleHeight;
          mScroll = Clamp( mouseDY, scrollMin, scrollMax );
        }

        if( !gKeyboardInput.IsKeyDown( Key::MouseLeft ) )
          mScrolling = false;
      }
      else if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) &&
               IsHovered( ImGuiRect::FromMinMax( mini, maxi ) ) )
      {
        Errors mouseErrors;
        v2 mousePosScreenspace;
        OS::GetScreenspaceCursorPos( mousePosScreenspace, mouseErrors );
        if( mouseErrors.empty() )
        {
          mScrolling = true;
          mScrollMousePosScreenspaceInitial = mousePosScreenspace;
        }
      }

      mContentRect = ImGuiRect::FromPosSize( mPosViewport, v2( mSize.x - scrollbarWidth, mSize.y ) );
    }
    else
    {
      mContentRect = ImGuiRect::FromPosSize( mPosViewport, mSize );
    }

    v2 padVec = v2( 1, 1 ) * ImGuiGlobals::Instance.mUIStyle.windowPadding;
    mContentRect.mMini += padVec;
    mContentRect.mMaxi -= padVec;

    mXOffsets.resize( 0 );
    mXOffsets.push_back( ImGuiGlobals::Instance.mUIStyle.windowPadding );
    v2 drawPos;
    drawPos.x = mPosViewport.x + mXOffsets.back();
    drawPos.y = mPosViewport.y + ImGuiGlobals::Instance.mUIStyle.windowPadding - mScroll;
    mCurrCursorViewport = drawPos;
    mPrevCursorViewport = drawPos;
    mMaxiCursorViewport = drawPos;
    mCurrLineHeight = 0;
    mPrevLineHeight = 0;

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

    //if( mActiveIDPrev && !mActiveID )
    //{
    //}
    //mActiveIDPrev = mActiveID;
  }

  void ImGuiWindow::ComputeClipInfo( bool* clipped,
                                     ImGuiRect* clipRect )
  {
    const ImGuiRect windowRect = ImGuiRect::FromPosSize( mPosViewport, mSize );
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
    UpdateMaxCursorDrawPos( mCurrCursorViewport + v2{ size.x, mCurrLineHeight } );

    mPrevCursorViewport = mCurrCursorViewport + v2( size.x, 0 );
    mPrevLineHeight = mCurrLineHeight;

    mCurrCursorViewport.x = mPosViewport.x + mXOffsets.back();
    mCurrCursorViewport.y += mCurrLineHeight + ImGuiGlobals::Instance.mUIStyle.itemSpacing.y;
    mCurrLineHeight = 0;
  }

  void ImGuiWindow::UpdateMaxCursorDrawPos( v2 pos )
  {
    mMaxiCursorViewport.x = Max( mMaxiCursorViewport.x, pos.x );
    mMaxiCursorViewport.y = Max( mMaxiCursorViewport.y, pos.y );
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

  v2 ImGuiWindow::GetMousePosViewport()
  {
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    const v2 result
      = gKeyboardInput.mCurr.mScreenspaceCursorPos
      - v2( ( float )desktopWindowState->mX, ( float )desktopWindowState->mY );
    return result;
  }

  void* ImGuiWindow::GetWindowResource( ImGuiindex index )
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
