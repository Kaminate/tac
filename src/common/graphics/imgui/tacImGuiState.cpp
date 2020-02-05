#include "common/graphics/imgui/tacImGuiState.h"
#include "common/graphics/tacTextEdit.h"
#include "common/graphics/tacUI2D.h"
#include "common/tackeyboardinput.h"
#include "common/tacOS.h"
#include "common/math/tacMath.h"

TacImGuiGlobals TacImGuiGlobals::Instance;

TacImGuiWindow::TacImGuiWindow()
{
  inputData = new TacTextInputData;
}
TacImGuiWindow::~TacImGuiWindow()
{
  delete inputData;
}
void TacImGuiWindow::BeginFrame()
{
  TacUI2DDrawData* ui2DDrawData = TacImGuiGlobals::Instance.mUI2DDrawData;
  TacKeyboardInput* keyboardInput = TacImGuiGlobals::Instance.mKeyboardInput;


  if( mParent )
  {
    mPos = mParent->mCurrCursorDrawPos;
    // Render borders
    {
      bool clipped;
      auto clipRect = TacImGuiRect::FromPosSize( mPos, mSize );
      ComputeClipInfo( &clipped, &clipRect );
      if( !clipped )
      {
        v4 childWindowColor = v4( 0.1f, 0.15f, 0.2f, 1.0f );
        ui2DDrawData->AddBox( mPos, mPos + mSize, childWindowColor, nullptr, nullptr );
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
    ui2DDrawData->AddBox( mini, maxi, scrollbarBackgroundColor, nullptr, nullptr );

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
    ui2DDrawData->AddBox( mini, maxi, scrollbarForegroundColor, nullptr, nullptr );


    if( mScrolling )
    {
      TacErrors mouseErrors;
      v2 mousePosScreenspace;
      TacOS::Instance->GetScreenspaceCursorPos( mousePosScreenspace, mouseErrors );
      if( mouseErrors.empty() )
      {
        float mouseDY = mousePosScreenspace.y - mScrollMousePosScreenspaceInitial.y;
        float scrollMin = 0;
        float scrollMax = contentAllHeight - contentVisibleHeight;
        mScroll = TacClamp( mouseDY, scrollMin, scrollMax );
      }

      if( !TacKeyboardInput::Instance->IsKeyDown( TacKey::MouseLeft ) )
        mScrolling = false;
    }
    else if( TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) &&
      TacImGuiGlobals::Instance.IsHovered( TacImGuiRect::FromMinMax( mini, maxi ) ) )
    {
      TacErrors mouseErrors;
      v2 mousePosScreenspace;
      TacOS::Instance->GetScreenspaceCursorPos( mousePosScreenspace, mouseErrors );
      if( mouseErrors.empty() )
      {
        mScrolling = true;
        mScrollMousePosScreenspaceInitial = mousePosScreenspace;
      }
    }

    mContentRect = TacImGuiRect::FromPosSize( mPos, v2( mSize.x - scrollbarWidth, mSize.y ) );
  }
  else
  {
    mContentRect = TacImGuiRect::FromPosSize( mPos, mSize );
  }

  v2 padVec = v2( 1, 1 ) * TacImGuiGlobals::Instance.mUIStyle.windowPadding;
  mContentRect.mMini += padVec;
  mContentRect.mMaxi -= padVec;

  mXOffsets = { TacImGuiGlobals::Instance.mUIStyle.windowPadding };
  v2 drawPos = {
    //       +----- grody ------+
    //       |                  |
    //       v                  v
    mPos.x + mXOffsets.back(),
    mPos.y + TacImGuiGlobals::Instance.mUIStyle.windowPadding - mScroll };
  mCurrCursorDrawPos = drawPos;
  mPrevCursorDrawPos = drawPos;
  mMaxiCursorDrawPos = drawPos;
  mCurrLineHeight = 0;
  mPrevLineHeight = 0;

  if( !mParent )
  {
    if( !mIDAllocator )
      mIDAllocator = new TacImGuiIDAllocator;
    mIDAllocator->mIDCounter = 0;
    if( !TacImGuiGlobals::Instance.mIsWindowDirectlyUnderCursor &&
      TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) )
    {
      mIDAllocator->mActiveID = TacImGuiIdNull;
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
void TacImGuiWindow::ComputeClipInfo( bool* clipped, TacImGuiRect* clipRect )
{
  auto windowRect = TacImGuiRect::FromPosSize( mPos, mSize );
  *clipped =
    clipRect->mMini.x > windowRect.mMaxi.x ||
    clipRect->mMaxi.x < windowRect.mMini.x ||
    clipRect->mMini.y > windowRect.mMaxi.y ||
    clipRect->mMaxi.y < windowRect.mMini.y;
  clipRect->mMini.x = TacMax( clipRect->mMini.x, windowRect.mMini.x );
  clipRect->mMini.y = TacMax( clipRect->mMini.y, windowRect.mMini.y );
  clipRect->mMaxi.x = TacMin( clipRect->mMaxi.x, windowRect.mMaxi.x );
  clipRect->mMaxi.y = TacMin( clipRect->mMaxi.y, windowRect.mMaxi.y );
}
void TacImGuiWindow::ItemSize( v2 size )
{
  mCurrLineHeight = TacMax( mCurrLineHeight, size.y );
  UpdateMaxCursorDrawPos( mCurrCursorDrawPos + v2{ size.x, mCurrLineHeight } );

  mPrevCursorDrawPos = mCurrCursorDrawPos + v2( size.x, 0 );
  mPrevLineHeight = mCurrLineHeight;

  mCurrCursorDrawPos.x = mPos.x + mXOffsets.back();
  mCurrCursorDrawPos.y += mCurrLineHeight + TacImGuiGlobals::Instance.mUIStyle.itemSpacing.y;
  mCurrLineHeight = 0;
}
void TacImGuiWindow::UpdateMaxCursorDrawPos( v2 pos )
{
  mMaxiCursorDrawPos.x = TacMax( mMaxiCursorDrawPos.x, pos.x );
  mMaxiCursorDrawPos.y = TacMax( mMaxiCursorDrawPos.y, pos.y );
}
TacImGuiId  TacImGuiWindow::GetActiveID()
{
  return mIDAllocator->mActiveID;
}
void TacImGuiWindow::SetActiveID( TacImGuiId id )
{
  mIDAllocator->mActiveID = id;
}
TacImGuiId TacImGuiWindow::GetID()
{
  return mIDAllocator->mIDCounter++;
}

TacImGuiWindow* TacImGuiGlobals::FindWindow( const TacString& name )
{
  for( TacImGuiWindow* window : mAllWindows )
    if( window->mName == name )
      return window;
  return nullptr;
}
bool TacImGuiGlobals::IsHovered( const TacImGuiRect& rect )
{
  if( !mIsWindowDirectlyUnderCursor )
    return false;
  return
    mMousePositionDesktopWindowspace.x > rect.mMini.x &&
    mMousePositionDesktopWindowspace.x < rect.mMaxi.x &&
    mMousePositionDesktopWindowspace.y > rect.mMini.y &&
    mMousePositionDesktopWindowspace.y < rect.mMaxi.y;
}
