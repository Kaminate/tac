#include "src/common/graphics/imgui/tacImGuiState.h"
#include "src/common/graphics/tacTextEdit.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/math/tacMath.h"

namespace Tac
{

  struct RegisteredWindowResource
  {
    String mName;
    Vector<char> mInitialData;
    ImGuiResourceId mId;
  };

  struct WindowResourceRegistry
  {
    static WindowResourceRegistry* GetInstance();
    ImGuiResourceId                RegisterResource( StringView name,
                                                     void* initialDataBytes,
                                                     int initialDataByteCount );
    RegisteredWindowResource*      FindResource( ImGuiResourceId  resourceId );
  private:
    Vector< RegisteredWindowResource > mRegisteredWindowResources;
    int mResourceCounter = 0;
  };
  RegisteredWindowResource* WindowResourceRegistry::FindResource( ImGuiResourceId  resourceId )
  {
    for( RegisteredWindowResource& resource : mRegisteredWindowResources )
      if( resource.mId == resourceId )
        return &resource;
    return nullptr;
  }
  WindowResourceRegistry* WindowResourceRegistry::GetInstance()
  {
    static WindowResourceRegistry instance;
    return &instance;
  }

  ImGuiResourceId WindowResourceRegistry::RegisterResource( StringView name,
                                                            void* initialDataBytes,
                                                            int initialDataByteCount )
  {
    const char* dataBegin = ( char* )initialDataBytes;
    const char* dataEnd = ( char* )initialDataBytes + initialDataByteCount;
    const ImGuiResourceId id = mResourceCounter++;
    RegisteredWindowResource resource;
    resource.mInitialData = Vector< char >( dataBegin, dataEnd );
    resource.mName = name;
    resource.mId = id;
    mRegisteredWindowResources.push_back( resource );
    return id;
  }

  ImGuiResourceId ImGuiRegisterWindowResource( StringView name,
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
    inputData = TAC_NEW TextInputData;
  }
  ImGuiWindow::~ImGuiWindow()
  {
    delete inputData;
  }
  void ImGuiWindow::BeginFrame()
  {
    UI2DDrawData* ui2DDrawData = ImGuiGlobals::Instance.mUI2DDrawData;
    KeyboardInput* keyboardInput = KeyboardInput::Instance;


    if( mParent )
    {
      mPos = mParent->mCurrCursorDrawPos;
      // Render borders
      {
        bool clipped;
        auto clipRect = ImGuiRect::FromPosSize( mPos, mSize );
        ComputeClipInfo( &clipped, &clipRect );
        if( !clipped )
        {
          v4 childWindowColor = v4( 0.1f, 0.15f, 0.2f, 1.0f );
          Render::TextureHandle texture;
          ui2DDrawData->AddBox( mPos, mPos + mSize, childWindowColor, texture, nullptr );
        }
      }

    }

    // Scrollbar
    if( mMaxiCursorDrawPos.y > mPos.y + mSize.y || mScroll )
    {
      float scrollbarWidth = 30;
      v2 mini = {
        mPos.x + mSize.x - scrollbarWidth,
        mPos.y };
      v2 maxi = mPos + mSize;
      v4 scrollbarBackgroundColor = v4( 0.4f, 0.2f, 0.8f, 1.0f );
      Render::TextureHandle invalidTexture;

      ui2DDrawData->AddBox( mini, maxi, scrollbarBackgroundColor, invalidTexture, nullptr );

      float contentAllMinY = mPos.y - mScroll;
      float contentAllMaxY = mMaxiCursorDrawPos.y;
      float contentAllHeight = contentAllMaxY - contentAllMinY;
      float contentVisibleMinY = mPos.y;
      float contentVisibleMaxY = mPos.y + mSize.y;
      float contentVisibleHeight = contentVisibleMaxY - contentVisibleMinY;

      mini.y = mPos.y + ( ( contentVisibleMinY - contentAllMinY ) / contentAllHeight ) * mSize.y;
      maxi.y = mPos.y + ( ( contentVisibleMaxY - contentAllMinY ) / contentAllHeight ) * mSize.y;

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

        if( !KeyboardInput::Instance->IsKeyDown( Key::MouseLeft ) )
          mScrolling = false;
      }
      else if( KeyboardInput::Instance->IsKeyJustDown( Key::MouseLeft ) &&
               ImGuiGlobals::Instance.IsHovered( ImGuiRect::FromMinMax( mini, maxi ) ) )
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

      mContentRect = ImGuiRect::FromPosSize( mPos, v2( mSize.x - scrollbarWidth, mSize.y ) );
    }
    else
    {
      mContentRect = ImGuiRect::FromPosSize( mPos, mSize );
    }

    v2 padVec = v2( 1, 1 ) * ImGuiGlobals::Instance.mUIStyle.windowPadding;
    mContentRect.mMini += padVec;
    mContentRect.mMaxi -= padVec;

    mXOffsets.resize( 0 );
    mXOffsets.push_back( ImGuiGlobals::Instance.mUIStyle.windowPadding );
    v2 drawPos = {
      //       +----- grody ------+
      //       |                  |
      //       v                  v
      mPos.x + mXOffsets.back(),
      mPos.y + ImGuiGlobals::Instance.mUIStyle.windowPadding - mScroll };
    mCurrCursorDrawPos = drawPos;
    mPrevCursorDrawPos = drawPos;
    mMaxiCursorDrawPos = drawPos;
    mCurrLineHeight = 0;
    mPrevLineHeight = 0;

    if( !mParent )
    {
      if( !mIDAllocator )
        mIDAllocator = TAC_NEW ImGuiIDAllocator;
      mIDAllocator->mIDCounter = 0;
      if( !ImGuiGlobals::Instance.mIsWindowDirectlyUnderCursor &&
          KeyboardInput::Instance->IsKeyJustDown( Key::MouseLeft ) )
      {
        mIDAllocator->mActiveID = ImGuiIdNull;
      }
    }
    else
    {
      mIDAllocator = mParent->mIDAllocator;
    }


    //if( mActiveIDPrev && !mActiveID )
    //{
    //}
    //mActiveIDPrev = mActiveID;
  }
  void ImGuiWindow::ComputeClipInfo( bool* clipped,
                                     ImGuiRect* clipRect )
  {
    auto windowRect = ImGuiRect::FromPosSize( mPos, mSize );
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
    UpdateMaxCursorDrawPos( mCurrCursorDrawPos + v2{ size.x, mCurrLineHeight } );

    mPrevCursorDrawPos = mCurrCursorDrawPos + v2( size.x, 0 );
    mPrevLineHeight = mCurrLineHeight;

    mCurrCursorDrawPos.x = mPos.x + mXOffsets.back();
    mCurrCursorDrawPos.y += mCurrLineHeight + ImGuiGlobals::Instance.mUIStyle.itemSpacing.y;
    mCurrLineHeight = 0;
  }
  void ImGuiWindow::UpdateMaxCursorDrawPos( v2 pos )
  {
    mMaxiCursorDrawPos.x = Max( mMaxiCursorDrawPos.x, pos.x );
    mMaxiCursorDrawPos.y = Max( mMaxiCursorDrawPos.y, pos.y );
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


  void* ImGuiWindow::GetWindowResource( ImGuiResourceId resourceId )
  {
    ImGuiId imGuiId = GetID();
    for( ImGuiWindowResource& resource : mResources )
      if( resource.mImGuiId == imGuiId && resource.mResourceId == resourceId )
        return resource.mData.data();

    RegisteredWindowResource* pRegistered = WindowResourceRegistry::GetInstance()->FindResource( resourceId );
    TAC_ASSERT( pRegistered );

    mResources.resize( mResources.size() + 1 );
    ImGuiWindowResource& resource = mResources.back();
    resource.mData = pRegistered->mInitialData;
    resource.mImGuiId = imGuiId;
    resource.mResourceId = resourceId;
    return resource.mData.data();
  }

  ImGuiWindow* ImGuiGlobals::FindWindow( StringView name )
  {
    for( ImGuiWindow* window : mAllWindows )
      if( window->mName == name )
        return window;
    return nullptr;
  }
  bool ImGuiGlobals::IsHovered( const ImGuiRect& rect )
  {
    if( !mIsWindowDirectlyUnderCursor )
      return false;
    return
      mMousePositionDesktopWindowspace.x > rect.mMini.x &&
      mMousePositionDesktopWindowspace.x < rect.mMaxi.x &&
      mMousePositionDesktopWindowspace.y > rect.mMini.y &&
      mMousePositionDesktopWindowspace.y < rect.mMaxi.y;
  }
}
