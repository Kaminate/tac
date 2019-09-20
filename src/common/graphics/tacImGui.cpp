#include "common/math/tacVector4.h"
#include "common/math/tacMath.h"
#include "common/graphics/tacUI.h"
#include "common/graphics/tacImGui.h"
#include "common/graphics/tacUI2D.h"
#include "common/graphics/tacTextEdit.h"
#include "common/graphics/tacRenderer.h"
#include "common/tacDesktopWindow.h"
#include "common/tackeyboardinput.h"
#include "common/tacOS.h"
#include "common/tacPreprocessor.h"
#include "common/tacShell.h"
#include <cstdlib> // atof

typedef int TacImGuiId;
const int TacImGuiIdNull = -1;

enum class TacDragMode
{
  Drag,
  TextInput
};

struct TacDragData
{
  TacDragMode mMode = ( TacDragMode )0;
  float mDragDistPx = 0;

  // This is the value of the variable we are gizmoing for at the start of the mouse drag.
  // That way, we can accumulate all the mouse drag pixels and apply them to ints in
  // addition to floats
  TacVector< char > mValueCopy;
};

struct TacImGuiIDAllocator
{
  TacImGuiId mActiveID = TacImGuiIdNull;
  TacImGuiId mIDCounter = 0;
};

struct TacGroupData
{
  // Position of the cursor when starting the group
  v2 mSavedCursorDrawPos;
  float mSavedLineHeight;
};

struct TacImGuiWindow
{
  TacImGuiWindow();
  ~TacImGuiWindow();
  void BeginFrame();
  void ItemSize( v2 size );
  void ComputeClipInfo( bool* clipped, TacImGuiRect* clipRect );
  void UpdateMaxCursorDrawPos( v2 pos );
  TacImGuiId GetID();
  void SetActiveID( TacImGuiId );
  TacImGuiId  GetActiveID();

  TacString mName;
  TacImGuiWindow* mParent = nullptr;

  // The most bottom right the cursor has ever been,
  // updated during ItemSize()
  v2 mMaxiCursorDrawPos;
  v2 mCurrCursorDrawPos;
  v2 mPrevCursorDrawPos;

  // ( rename to mPosDesktopWindowspace )
  // Position of this TacImGuiWindow relative to the desktop window (?)
  v2 mPos = {};
  v2 mSize = {};
  TacImGuiRect mContentRect;
  float mCurrLineHeight = 0;
  float mPrevLineHeight = 0;

  float mScroll = 0;
  v2 mScrollMousePosScreenspaceInitial;
  bool mScrolling = false;

  TacVector< TacGroupData > mGroupStack;

  // The mXOffsets.back() represents the horizontal tabbing distance
  // from the window mPos and the stuff that's about to be drawn
  TacVector< float > mXOffsets;

  // Shared between sub-windows
  TacImGuiIDAllocator* mIDAllocator = nullptr;

  TacTextInputData* inputData;
  std::map< TacImGuiId, bool > mCollapsingHeaderStates;

  std::map< TacImGuiId, TacDragData > mDragDatas;

  bool mIsAppendingToMenu = false;
};


struct TacUIStyle
{
  float windowPadding = 8;
  v2 itemSpacing = { 8, 4 };

  // Should this be a float?
  int fontSize = 16;

  float buttonPadding = 3.0f;
  v4 textColor = { 1, 1, 0, 1 };
} gStyle;

TacImGuiGlobals gTacImGuiGlobals;
static TacVector< int > fontsizestack;

static int TacGetCaret(
  TacUI2DDrawData* drawData,
  const TacVector< TacCodepoint >& codepoints,
  float mousePos ) // mouse pos rel text top left corner
{
  auto codepointCount = ( int )codepoints.size();
  float runningTextWidth = 0;
  int numGlyphsBeforeCaret = 0;
  for( int i = 1; i <= codepointCount; ++i )
  {
    v2 substringSize = drawData->CalculateTextSize(
      codepoints.begin(), i, gStyle.fontSize );

    float lastGlyphWidth = substringSize.x - runningTextWidth;
    float lastGlyphMidpoint = runningTextWidth + lastGlyphWidth / 2;

    if( mousePos < lastGlyphMidpoint )
      break;

    runningTextWidth += lastGlyphWidth;
    numGlyphsBeforeCaret++;
  }
  return numGlyphsBeforeCaret;
}


static void TacTextInputDataUpdateKeys( TacTextInputData* inputData, v2 textPos )
{
  TacKeyboardInput* keyboardInput = gTacImGuiGlobals.mKeyboardInput;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  static std::map< TacKey, TacTextInputKey > foo =
  {
    { TacKey::LeftArrow, TacTextInputKey::LeftArrow },
  { TacKey::RightArrow, TacTextInputKey::RightArrow },
  { TacKey::Backspace, TacTextInputKey::Backspace },
  { TacKey::Delete, TacTextInputKey::Delete },
  };
  for( auto pair : foo )
    if( TacKeyboardInput::Instance->IsKeyJustDown( pair.first ) )
      inputData->OnKeyPressed( pair.second );
  if( TacKeyboardInput::Instance->mWMCharPressedHax )
    inputData->OnCodepoint( TacKeyboardInput::Instance->mWMCharPressedHax );
  if( TacKeyboardInput::Instance->mCurr.IsKeyDown( TacKey::MouseLeft ) )
  {
    float mousePositionTextSpace = gTacImGuiGlobals.mMousePositionDesktopWindowspace.x - textPos.x;
    int numGlyphsBeforeCaret = TacGetCaret( drawData, inputData->mCodepoints, mousePositionTextSpace );

    if( TacKeyboardInput::Instance->mPrev.IsKeyDown( TacKey::MouseLeft ) )
      inputData->OnDrag( numGlyphsBeforeCaret );
    else
      inputData->OnClick( numGlyphsBeforeCaret );
  }
}


static void TacTextInputDataDrawSelection( TacTextInputData* inputData, v2 textPos, const TacImGuiRect* clipRect )
{
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  if( inputData->mCaretCount == 2 )
  {
    float minCaretPos = drawData->CalculateTextSize(
      inputData->mCodepoints.data(),
      inputData->GetMinCaret(),
      gStyle.fontSize ).x;

    float maxCaretPos = drawData->CalculateTextSize(
      inputData->mCodepoints.data(),
      inputData->GetMaxCaret(),
      gStyle.fontSize ).x;

    v2 selectionMini = {
      textPos.x + minCaretPos,
      textPos.y };
    v2 selectionMaxi = {
      textPos.x + maxCaretPos,
      textPos.y + gStyle.fontSize };

    drawData->AddBox( selectionMini, selectionMaxi, { 0.5f, 0.5f, 0.0f, 1 }, nullptr, clipRect );
  }

  if( inputData->mCaretCount == 1 )
  {
    float caretPos = drawData->CalculateTextSize(
      inputData->mCodepoints.data(),
      inputData->mNumGlyphsBeforeCaret[ 0 ],
      gStyle.fontSize ).x;
    float caretYPadding = 2.0f;
    float caretHalfWidth = 0.5f;
    v2 caretMini = {
      textPos.x + caretPos - caretHalfWidth,
      textPos.y + caretYPadding };
    v2 caretMaxi = {
      textPos.x + caretPos + caretHalfWidth,
      textPos.y + gStyle.fontSize - caretYPadding };
    float caretColorAlpha = ( ( std::sin( 6.0f * ( float )gTacImGuiGlobals.mElapsedSeconds ) + 1.0f ) / 2.0f );
    v4 caretColor = { 0, 0, 0, caretColorAlpha };
    drawData->AddBox( caretMini, caretMaxi, caretColor, nullptr, clipRect );
  }
}


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
  TacUI2DDrawData* ui2DDrawData = gTacImGuiGlobals.mUI2DDrawData;
  TacKeyboardInput* keyboardInput = gTacImGuiGlobals.mKeyboardInput;


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
      gTacImGuiGlobals.IsHovered( TacImGuiRect::FromMinMax( mini, maxi ) ) )
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

  v2 padVec = v2( 1, 1 ) * gStyle.windowPadding;
  mContentRect.mMini += padVec;
  mContentRect.mMaxi -= padVec;

  mXOffsets = { gStyle.windowPadding };
  v2 drawPos = {
    //       +----- grody ------+
    //       |                  |
    //       v                  v
    mPos.x + mXOffsets.back(),
    mPos.y + gStyle.windowPadding - mScroll };
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
    if( !gTacImGuiGlobals.mIsWindowDirectlyUnderCursor &&
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
  if( clipRect->mMini.x > windowRect.mMaxi.x ||
    clipRect->mMaxi.x < windowRect.mMini.x ||
    clipRect->mMini.y > windowRect.mMaxi.y ||
    clipRect->mMaxi.y < windowRect.mMini.y )
  {
    *clipped = true;
    return;
  }

  clipRect->mMini.x = TacMax( clipRect->mMini.x, windowRect.mMini.x );
  clipRect->mMini.y = TacMax( clipRect->mMini.y, windowRect.mMini.y );
  clipRect->mMaxi.x = TacMin( clipRect->mMaxi.x, windowRect.mMaxi.x );
  clipRect->mMaxi.y = TacMin( clipRect->mMaxi.y, windowRect.mMaxi.y );
  *clipped = false;
}
void TacImGuiWindow::ItemSize( v2 size )
{
  mCurrLineHeight = TacMax( mCurrLineHeight, size.y );
  UpdateMaxCursorDrawPos( mCurrCursorDrawPos + v2{ size.x, mCurrLineHeight } );

  mPrevCursorDrawPos = mCurrCursorDrawPos + v2( size.x, 0 );
  mPrevLineHeight = mCurrLineHeight;

  mCurrCursorDrawPos.x = mPos.x + mXOffsets.back();
  mCurrCursorDrawPos.y += mCurrLineHeight + gStyle.itemSpacing.y;
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
static bool AreEqual(
  const TacVector< TacCodepoint >& a,
  const TacVector< TacCodepoint >& b )
{
  int aSize = a.size();
  if( aSize != b.size() )
    return false;
  for( int i = 0; i < aSize; ++i )
    if( a[ i ] != b[ i ] )
      return false;
  return true;
}

TacImGuiRect TacImGuiRect::FromPosSize( v2 pos, v2 size )
{
  TacImGuiRect result;
  result.mMini = pos;
  result.mMaxi = pos + size;
  return result;
}
TacImGuiRect TacImGuiRect::FromMinMax( v2 mini, v2 maxi )
{
  TacImGuiRect result;
  result.mMini = mini;
  result.mMaxi = maxi;
  return result;
}
float TacImGuiRect::GetWidth()
{
  return mMaxi.x - mMini.x;
}
float TacImGuiRect::GetHeight()
{
  return mMaxi.y - mMini.y;
}
v2 TacImGuiRect::GetDimensions()
{
  return mMaxi - mMini;
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

void TacImGuiSetNextWindowPos( v2 pos )
{
  gTacImGuiGlobals.mNextWindowPos = pos;
}
void TacImGuiBegin( const TacString& name, v2 size )
{
  TacImGuiWindow* window = gTacImGuiGlobals.FindWindow( name );
  if( !window )
  {
    window = new TacImGuiWindow;
    window->mName = name;
    gTacImGuiGlobals.mAllWindows.push_back( window );
  }
  if( gTacImGuiGlobals.mNextWindowPos != v2( 0, 0 ) )
  {
    window->mPos = gTacImGuiGlobals.mNextWindowPos;
    gTacImGuiGlobals.mNextWindowPos = {};
  }
  const TacImage& image = gTacImGuiGlobals.mUI2DDrawData->mRenderView->mFramebuffer->myImage;
  window->mSize = {
    size.x > 0 ? size.x : size.x + image.mWidth,
    size.y > 0 ? size.y : size.y + image.mHeight };
  TacAssert( gTacImGuiGlobals.mWindowStack.empty() );
  gTacImGuiGlobals.mWindowStack = { window };
  gTacImGuiGlobals.mCurrentWindow = window;
  window->BeginFrame();
}
void TacImGuiEnd()
{
  gTacImGuiGlobals.mWindowStack.pop_back();
  gTacImGuiGlobals.mCurrentWindow =
    gTacImGuiGlobals.mWindowStack.empty() ? nullptr :
    gTacImGuiGlobals.mWindowStack.back();
}
void TacImGuiBeginChild( const TacString& name, v2 size )
{
  TacImGuiWindow* child = gTacImGuiGlobals.FindWindow( name );
  TacImGuiWindow* parent = gTacImGuiGlobals.mCurrentWindow;
  if( !child )
  {
    child = new TacImGuiWindow;
    child->mName = name;
    child->mParent = parent;
    gTacImGuiGlobals.mAllWindows.push_back( child );
  }
  child->mSize = {
    size.x > 0 ? size.x : size.x + parent->mSize.x,
    size.y > 0 ? size.y : size.y + parent->mSize.y };
  gTacImGuiGlobals.mWindowStack.push_back( child );
  gTacImGuiGlobals.mCurrentWindow = child;
  child->BeginFrame();
}
void TacImGuiEndChild()
{
  TacImGuiWindow* child = gTacImGuiGlobals.mCurrentWindow;
  child->mParent->ItemSize( child->mSize );
  gTacImGuiGlobals.mWindowStack.pop_back();
  gTacImGuiGlobals.mCurrentWindow = gTacImGuiGlobals.mWindowStack.back();
}
void TacImGuiBeginGroup()
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacGroupData groupData = {};
  groupData.mSavedCursorDrawPos = window->mCurrCursorDrawPos;
  groupData.mSavedLineHeight = window->mCurrLineHeight;

  window->mXOffsets.push_back( window->mCurrCursorDrawPos.x - window->mPos.x );
  window->mCurrLineHeight = 0;

  // new
  //window->mMaxiCursorDrawPos = window->mCurrCursorDrawPos;

  window->mGroupStack.push_back( groupData );
}
void TacImGuiEndGroup()
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacGroupData& groupData = window->mGroupStack.back();
  window->mXOffsets.pop_back();
  //v2 groupEndPos = {
  //  window->mMaxiCursorDrawPos.x,
  //  window->mMaxiCursorDrawPos.y + window->mPrevLineHeight };
  //v2 groupSize = groupEndPos - groupData.mSavedCursorDrawPos;
  v2 groupSize = window->mMaxiCursorDrawPos - groupData.mSavedCursorDrawPos;
  //groupSize.y = TacMax( groupSize.y, window->mCurrLineHeight );

  window->mCurrLineHeight = groupData.mSavedLineHeight;

  window->mCurrCursorDrawPos = groupData.mSavedCursorDrawPos;
  window->ItemSize( groupSize );
  window->mGroupStack.pop_back();
}
void TacImGuiIndent()
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  window->mCurrCursorDrawPos.x += 15.0f;
  window->mXOffsets.push_back( window->mCurrCursorDrawPos.x - window->mPos.x );
}
void TacImGuiUnindent()
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  window->mXOffsets.pop_back();
  window->mCurrCursorDrawPos.x = window->mPos.x + window->mXOffsets.back();
}
void TacImGuiSameLine()
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  window->mCurrCursorDrawPos = {
    window->mPrevCursorDrawPos.x + gStyle.itemSpacing.x,
    window->mPrevCursorDrawPos.y };
  window->mCurrLineHeight = window->mPrevLineHeight;
  //window->mCurrLineHeight = window->mPrevLineHeight;
}
void TacImGuiText( const TacString& utf8 )
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  v2 textPos = window->mCurrCursorDrawPos;
  v2 textSize = drawData->CalculateTextSize( utf8, gStyle.fontSize );
  window->ItemSize( textSize );
  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( textPos, textSize );
  window->ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return;
  drawData->AddText( textPos, gStyle.fontSize, utf8, gStyle.textColor, &clipRect );
}
bool TacImGuiInputText( const TacString& label, TacString& text )
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacTextInputData* inputData = window->inputData;
  TacKeyboardInput* keyboardInput = gTacImGuiGlobals.mKeyboardInput;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  TacImGuiId id = window->GetID();

  bool textChanged = false;

  v2 pos = window->mCurrCursorDrawPos;

  // Word wrap?
  int lineCount = 1;
  for( char c : text )
    if( c == '\n' )
      lineCount++;

  v2 totalSize = {
    window->mContentRect.mMaxi.x - pos.x,
    ( float )lineCount * ( float )gStyle.fontSize };

  window->ItemSize( totalSize );

  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, totalSize );
  window->ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return textChanged;

  if( gTacImGuiGlobals.IsHovered( clipRect ) && TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) )
    window->SetActiveID( id );

  v3 textBackgroundColor3 = { 1, 1, 0 };

  v4 editTextColor = { 0, 0, 0, 1 };
  v2 textBackgroundMaxi = {
    pos.x + totalSize.x * ( 2.0f / 3.0f ),
    pos.y + totalSize.y };
  v2 textPos = {
    pos.x + gStyle.buttonPadding,
    pos.y };
  drawData->AddBox(
    pos,
    textBackgroundMaxi,
    v4( textBackgroundColor3, 1 ), nullptr, &clipRect );

  if( window->GetActiveID() == id )
  {
    TacVector< TacCodepoint > codepoints;
    TacErrors ignoredUTF8ConversionErrors;
    TacUTF8Converter::Convert( text, codepoints, ignoredUTF8ConversionErrors );
    if( !AreEqual( inputData->mCodepoints, codepoints ) )
    {
      inputData->mCodepoints = codepoints;
      inputData->mCaretCount = 0;
    }
    TacTextInputDataUpdateKeys( inputData, textPos );

    // handle double click
    static double lastMouseReleaseSeconds;
    static v2 lastMousePositionDesktopWindowspace;
    if( TacKeyboardInput::Instance->HasKeyJustBeenReleased( TacKey::MouseLeft ) &&
      gTacImGuiGlobals.IsHovered( clipRect ) &&
      !inputData->mCodepoints.empty() )
    {
      auto mouseReleaseSeconds = gTacImGuiGlobals.mElapsedSeconds;
      if( mouseReleaseSeconds - lastMouseReleaseSeconds < 0.5f &&
        lastMousePositionDesktopWindowspace == gTacImGuiGlobals.mMousePositionDesktopWindowspace )
      {
        inputData->mNumGlyphsBeforeCaret[ 0 ] = 0;
        inputData->mNumGlyphsBeforeCaret[ 1 ] = inputData->mCodepoints.size();
        inputData->mCaretCount = 2;
      }
      lastMouseReleaseSeconds = mouseReleaseSeconds;
      lastMousePositionDesktopWindowspace = gTacImGuiGlobals.mMousePositionDesktopWindowspace;
    }

    TacTextInputDataDrawSelection( inputData, textPos, &clipRect );



    TacString newText;
    TacUTF8Converter::Convert( inputData->mCodepoints, newText );
    if( text != newText )
    {
      text = newText;
      textChanged = true;
    }
  }

  drawData->AddText( textPos, gStyle.fontSize, text, editTextColor, &clipRect );

  v2 labelPos = {
    textBackgroundMaxi.x + gStyle.itemSpacing.x,
    pos.y };
  drawData->AddText( labelPos, gStyle.fontSize, label, gStyle.textColor, &clipRect );

  return textChanged;
}
bool TacImGuiSelectable( const TacString& str, bool selected )
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacKeyboardInput* keyboardInput = gTacImGuiGlobals.mKeyboardInput;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  bool clicked = false;
  v2 pos = window->mCurrCursorDrawPos;
  v2 buttonSize = {
    window->mContentRect.mMaxi.x - pos.x,
    ( float )gStyle.fontSize };

  window->ItemSize( buttonSize );
  auto id = window->GetID();

  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, buttonSize );
  window->ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return clicked;

  v3 color3 = v3( 0.7f, 0.3f, 0.3f ) * 0.7f;
  if( selected )
    color3 = ( color3 + v3( 1, 1, 1 ) ) * 0.3f;


  bool hovered = gTacImGuiGlobals.IsHovered( clipRect );
  if( hovered )
  {
    color3 /= 2.0f;
    if( TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) )
    {
      color3 /= 2.0f;
      clicked = true;
      window->SetActiveID ( id );
    }
  }

  v4 color( color3, 1 );

  drawData->AddBox( pos, pos + buttonSize, color, nullptr, &clipRect );
  drawData->AddText( pos, gStyle.fontSize, str, gStyle.textColor, &clipRect );
  return clicked;
}
bool TacImGuiButton( const TacString& str )
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacKeyboardInput* keyboardInput = gTacImGuiGlobals.mKeyboardInput;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  bool justClicked = false;
  v2 textSize = drawData->CalculateTextSize( str, gStyle.fontSize );
  v2 buttonSize = { textSize.x + 2 * gStyle.buttonPadding, textSize.y };
  v2 pos = window->mCurrCursorDrawPos;
  window->ItemSize( textSize );


  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, buttonSize );
  window->ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return justClicked;

  bool hovered = gTacImGuiGlobals.IsHovered( clipRect );

  v3 outerBoxColor = v3( .23f, .28f, .38f );
  if( hovered )
  {
    outerBoxColor /= 2.0f;
    if( TacKeyboardInput::Instance->IsKeyDown( TacKey::MouseLeft ) )
    {
      outerBoxColor /= 2.0f;
    }
    if( TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) )
    {
      justClicked = true;
    }
  }

  drawData->AddBox( pos, pos + buttonSize, v4( outerBoxColor, 1 ), nullptr, &clipRect );

  v2 textPos = {
    pos.x + gStyle.buttonPadding,
    pos.y };
  drawData->AddText( textPos, gStyle.fontSize, str, gStyle.textColor, &clipRect );

  return justClicked;
}
void TacImGuiCheckbox( const TacString& str, bool* value )
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacKeyboardInput* keyboardInput = gTacImGuiGlobals.mKeyboardInput;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  v2 pos = window->mCurrCursorDrawPos;

  v2 textSize = drawData->CalculateTextSize( str, gStyle.fontSize );

  float boxWidth = textSize.y;
  v2 boxSize = v2( 1, 1 ) * boxWidth;

  v2 totalSize = v2( boxWidth + gStyle.itemSpacing.x + textSize.x, textSize.y );
  window->ItemSize( totalSize );

  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, totalSize );
  window->ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return;

  bool hovered = gTacImGuiGlobals.IsHovered( clipRect );

  v4 outerBoxColor = v4( 1, 1, 0, 1 );
  if( hovered )
  {
    outerBoxColor = v4( 0.5f, 0.5f, 0, 1 );
    if( TacKeyboardInput::Instance->IsKeyDown( TacKey::MouseLeft ) )
    {
      outerBoxColor = v4( 0.3f, 0.3f, 0, 1 );
    }
    if( TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) )
    {
      *value = !*value;
    }
  }

  drawData->AddBox( pos, pos + boxSize, outerBoxColor, nullptr, &clipRect );
  if( *value )
  {
    v4 checkmarkColor = v4( outerBoxColor.xyz() / 2.0f, 1.0f );
    bool drawCheckmark = false;
    if( drawCheckmark )
    {

      // (0,0)-------+
      // |         3 |
      // |       //  |
      // | 0 __2 /   |
      // |   \  /    |
      // |     1     |
      // +-------(1,1)
      v2 p0 = { 0.2f, 0.4f };
      v2 p1 = { 0.45f, 0.9f };
      v2 p2 = { 0.45f, 0.60f };
      v2 p3 = { 0.9f, 0.1f };
      for( v2* point : { &p0, &p1, &p2, &p3 } )
      {
        point->x = pos.x + point->x * boxWidth;
        point->y = pos.y + point->y * boxWidth;
      }
      int iVert = drawData->mDefaultVertex2Ds.size();
      int iIndex = drawData->mDefaultIndex2Ds.size();
      drawData->mDefaultIndex2Ds.push_back( iVert + 0 );
      drawData->mDefaultIndex2Ds.push_back( iVert + 1 );
      drawData->mDefaultIndex2Ds.push_back( iVert + 2 );
      drawData->mDefaultIndex2Ds.push_back( iVert + 1 );
      drawData->mDefaultIndex2Ds.push_back( iVert + 3 );
      drawData->mDefaultIndex2Ds.push_back( iVert + 2 );
      drawData->mDefaultVertex2Ds.resize( iVert + 4 );
      TacUI2DVertex* defaultVertex2D = &drawData->mDefaultVertex2Ds[ iVert ];
      defaultVertex2D->mPosition = p0;
      defaultVertex2D++;
      defaultVertex2D->mPosition = p1;
      defaultVertex2D++;
      defaultVertex2D->mPosition = p2;
      defaultVertex2D++;
      defaultVertex2D->mPosition = p3;

      TacDefaultCBufferPerObject perObjectData = {};
      perObjectData.World = m4::Identity();
      perObjectData.Color = checkmarkColor;

      TacUI2DDrawCall drawCall;
      drawCall.mIIndexCount = 6;
      drawCall.mIIndexStart = iIndex;
      drawCall.mIVertexCount = 4;
      drawCall.mIVertexStart = iVert;
      drawCall.mShader = TacUI2DCommonData::Instance->mShader;
      drawCall.mUniformSource = TacTemporaryMemory( perObjectData );
      drawData->mDrawCall2Ds.push_back( drawCall );
    }
    else
    {
      v2 innerPadding = v2( 1, 1 ) * 3;
      drawData->AddBox( pos + innerPadding, pos + boxSize - innerPadding, checkmarkColor, nullptr, &clipRect );
    }
  }

  v2 textPos = {
    pos.x + boxWidth + gStyle.itemSpacing.x,
    pos.y };
  drawData->AddText( textPos, gStyle.fontSize, str, gStyle.textColor, &clipRect );
}

static bool TacImguiDragVal(
  const TacString& str,
  void* valueBytes,
  int valueByteCount,
  void( *valueToStringGetter )( TacString& to, const void* from ),
  void( *valueFromStringSetter )( const TacString& from, void* to ),
  void( *whatToDoWithMousePixel )( float mouseChangeSinceBeginningOfDrag, const void* valAtDragStart, void* curVal ) )
{
  v4 backgroundBoxColor = { 1, 1, 0, 1 };
  TacString valueStr;
  valueToStringGetter( valueStr, valueBytes );

  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacKeyboardInput* keyboardInput = gTacImGuiGlobals.mKeyboardInput;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  TacTextInputData* inputData = window->inputData;
  v2 pos = window->mCurrCursorDrawPos;
  v2 totalSize = {
    window->mContentRect.mMaxi.x - pos.x,
    ( float )gStyle.fontSize };
  window->ItemSize( totalSize );
  TacImGuiId id = window->GetID();
  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, totalSize );
  window->ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return false;

  TacDragData& dragFloatData = window->mDragDatas[ id ];

  // Only used to check if this function should return true/false because the value
  // changed or didnt change
  static TacVector< char > valueFrameCopy;
  valueFrameCopy.resize(valueByteCount);
  TacMemCpy( valueFrameCopy.data(), valueBytes, valueByteCount );
  v2 valuePos = {
    pos.x + gStyle.buttonPadding,
    pos.y };

  if( gTacImGuiGlobals.IsHovered( clipRect ) && TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) )
  {
    window->SetActiveID( id );
  }
  if( window->GetActiveID() == id )
  {
    if( dragFloatData.mMode == TacDragMode::Drag )
    {
      static float lastMouseXDesktopWindowspace;
      if( gTacImGuiGlobals.IsHovered( clipRect ) )
        backgroundBoxColor.xyz() /= 2.0f;
      if( TacKeyboardInput::Instance->IsKeyDown( TacKey::MouseLeft ) )
      {
        backgroundBoxColor.xyz() /= 2.0f;
        if( TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) )
        {
          lastMouseXDesktopWindowspace = gTacImGuiGlobals.mMousePositionDesktopWindowspace.x;
          dragFloatData.mDragDistPx = 0;
          dragFloatData.mValueCopy.resize( valueByteCount );
          TacMemCpy( dragFloatData.mValueCopy.data(), valueBytes, valueByteCount );
        }

        if( TacKeyboardInput::Instance->mPrev.IsKeyDown( TacKey::MouseLeft ) )
        {
          float moveCursorDir = 0;
          if( gTacImGuiGlobals.mMousePositionDesktopWindowspace.x > clipRect.mMaxi.x )
            moveCursorDir = -1.0f;
          if( gTacImGuiGlobals.mMousePositionDesktopWindowspace.x < clipRect.mMini.x )
            moveCursorDir = 1.0f;
          if( moveCursorDir )
          {
            TacErrors cursorAccessErrors;
            v2 mousePosScreenspace;
            TacOS::Instance->GetScreenspaceCursorPos(
              mousePosScreenspace,
              cursorAccessErrors );
            if( cursorAccessErrors.empty() )
            {
              float xOffset = moveCursorDir * clipRect.GetWidth();
              gTacImGuiGlobals.mMousePositionDesktopWindowspace.x += xOffset;
              mousePosScreenspace.x += xOffset;
              TacOS::Instance->SetScreenspaceCursorPos(
                mousePosScreenspace,
                cursorAccessErrors );
            }
          }
          else
          {
            float dMousePx =
              gTacImGuiGlobals.mMousePositionDesktopWindowspace.x -
              lastMouseXDesktopWindowspace;
            dragFloatData.mDragDistPx += dMousePx;
            whatToDoWithMousePixel( dragFloatData.mDragDistPx, dragFloatData.mValueCopy.data(), valueBytes );
          }
        }
      }

      lastMouseXDesktopWindowspace = gTacImGuiGlobals.mMousePositionDesktopWindowspace.x;

      // handle double click
      static double lastMouseReleaseSeconds;
      static v2 lastMousePositionDesktopWindowspace;
      if( TacKeyboardInput::Instance->HasKeyJustBeenReleased( TacKey::MouseLeft ) &&
        gTacImGuiGlobals.IsHovered( clipRect ) )
      {
        auto mouseReleaseSeconds = gTacImGuiGlobals.mElapsedSeconds;
        if( mouseReleaseSeconds - lastMouseReleaseSeconds < 0.5f &&
          lastMousePositionDesktopWindowspace == gTacImGuiGlobals.mMousePositionDesktopWindowspace )
        {
          TacVector< TacCodepoint > codepoints;
          TacErrors ignoredUTF8ConversionErrors;
          TacUTF8Converter::Convert( valueStr, codepoints, ignoredUTF8ConversionErrors );
          inputData->mCodepoints = codepoints;
          inputData->mCaretCount = 2;
          inputData->mNumGlyphsBeforeCaret[ 0 ] = 0;
          inputData->mNumGlyphsBeforeCaret[ 1 ] = codepoints.size();
          dragFloatData.mMode = TacDragMode::TextInput;
        }
        lastMouseReleaseSeconds = mouseReleaseSeconds;
        lastMousePositionDesktopWindowspace = gTacImGuiGlobals.mMousePositionDesktopWindowspace;
      }
      //if( !TacKeyboardInput::Instance->IsKeyDown( TacKey::MouseLeft ) )
      //  window->mActiveID = TacImGuiIdNull;
    }

    if( dragFloatData.mMode == TacDragMode::TextInput )
    {
      TacTextInputDataUpdateKeys( inputData, valuePos );
      TacString newText;
      TacUTF8Converter::Convert( inputData->mCodepoints, newText );
      valueFromStringSetter( newText, valueBytes );
      valueStr = newText;

      if( TacKeyboardInput::Instance->IsKeyJustDown( TacKey::Tab ) )
        window->mIDAllocator->mActiveID++;
    }
  }

  if( dragFloatData.mMode == TacDragMode::TextInput && id != window->GetActiveID() )
  {
    dragFloatData.mMode = TacDragMode::Drag;
    dragFloatData.mDragDistPx = 0;
  }

  v2 backgroundBoxMaxi = {
    pos.x + totalSize.x * ( 2.0f / 3.0f ),
    pos.y + totalSize.y };
  drawData->AddBox( pos, backgroundBoxMaxi, backgroundBoxColor, nullptr, &clipRect );

  if( dragFloatData.mMode == TacDragMode::TextInput )
    TacTextInputDataDrawSelection( inputData, valuePos, &clipRect );
  drawData->AddText( valuePos, gStyle.fontSize, valueStr, v4( 0, 0, 0, 1 ), &clipRect );

  v2 labelPos = {
    backgroundBoxMaxi.x + gStyle.itemSpacing.x,
    pos.y };
  drawData->AddText( labelPos, gStyle.fontSize, str, gStyle.textColor, &clipRect );

  return TacMemCmp( valueFrameCopy.data(), valueBytes, valueByteCount );
}

bool TacImGuiDragFloat( const TacString& str, float* value )
{
  auto getter = []( TacString& to, const void* from )
  {
    float i =  *( ( float* )from );
    to = TacToString( i );
  };
  auto setter = []( const TacString& from, void* to )
  {
    float f = ( float )std::atof( from.c_str() );
    *( ( float* )to ) = f;
  };
  auto whatToDoWithMousePixel = []( float mouseChangeSinceBeginningOfDrag, const void* valAtDragStart, void* curVal )
  {
    const float& valAtDragStartRef = *( const float* )valAtDragStart;
    float& curValRef = *( float* )curVal;
    const float offset = ( float )( mouseChangeSinceBeginningOfDrag * 0.1f );
    curValRef = valAtDragStartRef + offset;
  };
  const bool result =  TacImguiDragVal( str, value, sizeof( float ), getter, setter, whatToDoWithMousePixel );
  return result;
}

bool TacImGuiDragInt( const TacString& str, int* value )
{
  auto getter = []( TacString& to, const void* from )
  {
    int i =  *( ( int* )from );
    to = TacToString( i );
  };
  auto setter = []( const TacString& from, void* to )
  {
    int i = std::atoi( from.c_str() );
    *( ( int* )to ) = i;
  };
  auto whatToDoWithMousePixel = []( float mouseChangeSinceBeginningOfDrag, const void* valAtDragStart, void* curVal )
  {
    const int& valAtDragStartRef = *( const int* )valAtDragStart;
    int& curValRef = *( int* )curVal;
    const int offset = ( int )( mouseChangeSinceBeginningOfDrag / 50.0f );
    curValRef = valAtDragStartRef + offset;
  };
  const bool result = TacImguiDragVal( str, value, sizeof( int ), getter, setter, whatToDoWithMousePixel );
  return result;
}

bool TacImGuiCollapsingHeader( const TacString& name )
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  TacKeyboardInput* keyboardInput = gTacImGuiGlobals.mKeyboardInput;
  v2 pos = window->mCurrCursorDrawPos;
  v2 totalSize = {
    window->mContentRect.mMaxi.x - pos.x,
    ( float )gStyle.fontSize };
  window->ItemSize( totalSize );
  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, totalSize );
  window->ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return false;

  v4 backgroundBoxColor = { 100 / 255.0f, 65 / 255.0f, 164 / 255.0f, 255 / 255.0f };
  TacImGuiId id = window->GetID();
  bool& isOpen = window->mCollapsingHeaderStates[ id ];
  if( gTacImGuiGlobals.IsHovered( clipRect ) )
  {
    backgroundBoxColor.xyz() /= 2.0f;
    if( TacKeyboardInput::Instance->IsKeyJustDown( TacKey::MouseLeft ) )
      isOpen = !isOpen;
  }

  drawData->AddBox( pos, pos + totalSize, backgroundBoxColor, nullptr, &clipRect );

  v4 textColor = { 1, 1, 1, 1 };
  v2 textPos = { pos.x + gStyle.buttonPadding, pos.y };
  drawData->AddText( textPos, gStyle.fontSize, isOpen ? "v" : ">", textColor, &clipRect );
  textPos.x += drawData->CalculateTextSize( "  ", gStyle.fontSize ).x;
  drawData->AddText( textPos, gStyle.fontSize, name, textColor, &clipRect );

  return isOpen;
}
void TacImGuiPushFontSize( int value )
{
  fontsizestack.push_back( gStyle.fontSize );
  gStyle.fontSize = value;
}
void TacImGuiPopFontSize()
{
  int fontsize = fontsizestack.back();
  gStyle.fontSize = fontsize;
  fontsizestack.pop_back();
}

void TacImGuiBeginMenuBar()
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;
  TacAssert( !window->mIsAppendingToMenu );
  window->mIsAppendingToMenu = true;
  v2 size = { window->mSize.x, gStyle.fontSize + gStyle.buttonPadding * 2 };
  v4 color = { v3( 69, 45, 83 ) / 255.0f, 1.0f };
  TacTexture* texture = nullptr;
  TacImGuiRect* cliprect = nullptr;
  drawData->AddBox( {}, size, color, texture, cliprect );
}
//void TacImGuiBeginMenu( const TacString& label )
//{
//  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
//  TacAssert( window->mIsAppendingToMenu );
//}
//void TacImGuiMenuItem( const TacString& label )
//{
//  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
//  TacAssert( window->mIsAppendingToMenu );
//}
//void TacImGuiEndMenu()
//{
//  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
//  TacAssert( window->mIsAppendingToMenu );
//}
void TacImGuiEndMenuBar()
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  TacAssert( window->mIsAppendingToMenu );
  window->mIsAppendingToMenu = false;
}

void TacImGuiDebugDraw()
{
  TacString text =
    "Cur window active id: " +
    TacToString( gTacImGuiGlobals.mCurrentWindow->GetActiveID() );
  TacImGuiText( text );
}

void TacImGuiSetGlobals(
  TacShell* shell,
  TacDesktopWindow* desktopWindow,
  TacUI2DDrawData* ui2DDrawData )
{
  TacErrors screenspaceCursorPosErrors;
  v2 screenspaceCursorPos;
  TacOS::Instance->GetScreenspaceCursorPos( screenspaceCursorPos, screenspaceCursorPosErrors );
  if( screenspaceCursorPosErrors.empty() )
  {
    gTacImGuiGlobals.mMousePositionDesktopWindowspace = {
      screenspaceCursorPos.x - desktopWindow->mX,
      screenspaceCursorPos.y - desktopWindow->mY };
    gTacImGuiGlobals.mIsWindowDirectlyUnderCursor = desktopWindow->mCursorUnobscured;
  }
  else
  {
    gTacImGuiGlobals.mIsWindowDirectlyUnderCursor = false;
  }
  gTacImGuiGlobals.mUI2DDrawData = ui2DDrawData;
  gTacImGuiGlobals.mElapsedSeconds = shell->mElapsedSeconds;
}

