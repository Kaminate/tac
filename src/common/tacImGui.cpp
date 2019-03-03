#include "common/tacImGui.h"
#include "common/math/tacVector4.h"
#include "common/math/tacMath.h"
#include "common/tacUI.h"
#include "common/tacUI2D.h"
#include "common/tackeyboardinput.h"
#include "common/tacDesktopWindow.h"
#include "common/tacOS.h"
#include "common/tacTextEdit.h"

TacImGuiGlobals gTacImGuiGlobals;

struct TacUIStyle
{
  float windowPadding = 8;
  v2 itemSpacing = { 8, 4 };

  // Should this be a float?
  int fontSize = 16;

  float buttonPadding = 3.0f;
  v4 textColor = { 1, 1, 0, 1 };
} gStyle;

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
    float contentVisiblePercent = contentVisibleHeight / contentAllHeight;


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

      if( !keyboardInput->IsKeyDown( TacKey::MouseLeft ) )
        mScrolling = false;
    }
    else if( keyboardInput->IsKeyJustDown( TacKey::MouseLeft ) &&
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
  mPrevLineHeight = TacMax( mCurrLineHeight, size.y );
  mPrevCursorDrawPos = {
    mCurrCursorDrawPos.x + size.x,
    mCurrCursorDrawPos.y };
  UpdateMaxCursorDrawPos( mPrevCursorDrawPos );
  mCurrCursorDrawPos = {
    mPos.x + mXOffsets.back(),
    mPrevCursorDrawPos.y + mPrevLineHeight + gStyle.itemSpacing.y };
  mCurrLineHeight = 0;
}
void TacImGuiWindow::UpdateMaxCursorDrawPos( v2 pos )
{
  mMaxiCursorDrawPos.x = TacMax( mMaxiCursorDrawPos.x, pos.x );
  mMaxiCursorDrawPos.y = TacMax( mMaxiCursorDrawPos.y, pos.y );
}

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
  if( !mIsWindowDirectlyCursor )
    return false;
  return
    mMousePositionDesktopWindowspace.x > rect.mMini.x &&
    mMousePositionDesktopWindowspace.x < rect.mMaxi.x &&
    mMousePositionDesktopWindowspace.y > rect.mMini.y &&
    mMousePositionDesktopWindowspace.y < rect.mMaxi.y;
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
    gTacImGuiGlobals.mWindowStack.size() ?
    gTacImGuiGlobals.mWindowStack.back() : nullptr;
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
  window->mGroupSavedCursorDrawPos = window->mCurrCursorDrawPos;
  window->mXOffsets.push_back( window->mCurrCursorDrawPos.x - window->mPos.x );
  window->mCurrLineHeight = 0;
}
void TacImGuiEndGroup()
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  window->mXOffsets.pop_back();
  v2 groupEndPos = {
    window->mMaxiCursorDrawPos.x,
    window->mMaxiCursorDrawPos.y + window->mPrevLineHeight };
  v2 groupSize = groupEndPos - window->mGroupSavedCursorDrawPos;
  window->mCurrCursorDrawPos = window->mGroupSavedCursorDrawPos;
  window->ItemSize( groupSize );
}
void TacImGuiSameLine()
{
  TacImGuiWindow* window = gTacImGuiGlobals.mCurrentWindow;
  window->mCurrCursorDrawPos = {
    window->mPrevCursorDrawPos.x + gStyle.itemSpacing.x,
    window->mPrevCursorDrawPos.y };
  window->mCurrLineHeight = window->mPrevLineHeight;
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
  TacKeyboardInput* keyboardInput = gTacImGuiGlobals.mKeyboardInput;
  TacUI2DDrawData* drawData = gTacImGuiGlobals.mUI2DDrawData;

  static TacTextInputData inputData;

  TacUTF8Converter converter;
  TacVector< TacCodepoint > codepoints;
  TacErrors ignoredUTF8ConversionErrors;
  converter.Convert( text, codepoints, ignoredUTF8ConversionErrors );
  if( !AreEqual( inputData.mCodepoints, codepoints ) )
  {
    inputData.mCodepoints = codepoints;
    inputData.mCaretCount = 0;
  }

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

  static std::map< TacKey, TacTextInputKey > foo =
  {
    { TacKey::LeftArrow, TacTextInputKey::LeftArrow },
  { TacKey::RightArrow, TacTextInputKey::RightArrow },
  { TacKey::Backspace, TacTextInputKey::Backspace },
  { TacKey::Delete, TacTextInputKey::Delete },
  };
  for( auto pair : foo )
    if( keyboardInput->IsKeyJustDown( pair.first ) )
      inputData.OnKeyPressed( pair.second );
  if( keyboardInput->mWMCharPressedHax )
    inputData.OnCodepoint( keyboardInput->mWMCharPressedHax );

  // handle double click
  static double lastMouseReleaseSeconds;
  static v2 lastMousePositionDesktopWindowspace;
  if( keyboardInput->HasKeyJustBeenReleased( TacKey::MouseLeft ) &&
    gTacImGuiGlobals.IsHovered( clipRect ) &&
    !inputData.mCodepoints.empty() )
  {
    auto mouseReleaseSeconds = gTacImGuiGlobals.mElapsedSeconds;
    if( mouseReleaseSeconds - lastMouseReleaseSeconds < 0.5f &&
      lastMousePositionDesktopWindowspace == gTacImGuiGlobals.mMousePositionDesktopWindowspace )
    {
      inputData.mNumGlyphsBeforeCaret[ 0 ] = 0;
      inputData.mNumGlyphsBeforeCaret[ 1 ] = inputData.mCodepoints.size();
      inputData.mCaretCount = 2;
    }
    lastMouseReleaseSeconds = mouseReleaseSeconds;
    lastMousePositionDesktopWindowspace = gTacImGuiGlobals.mMousePositionDesktopWindowspace;
  }

  if( inputData.mCaretCount == 2 )
  {
    float minCaretPos = drawData->CalculateTextSize(
      inputData.mCodepoints.data(),
      inputData.GetMinCaret(),
      gStyle.fontSize ).x;

    float maxCaretPos = drawData->CalculateTextSize(
      inputData.mCodepoints.data(),
      inputData.GetMaxCaret(),
      gStyle.fontSize ).x;

    v2 selectionMini = {
      textPos.x + minCaretPos,
      textPos.y };
    v2 selectionMaxi = {
      textPos.x + maxCaretPos,
      textPos.y + gStyle.fontSize };
    drawData->AddBox( selectionMini, selectionMaxi, v4( textBackgroundColor3 / 2, 1 ), nullptr, &clipRect );
  }

  drawData->AddText( textPos, gStyle.fontSize, text, editTextColor, &clipRect );

  v2 labelPos = {
    textBackgroundMaxi.x + gStyle.itemSpacing.x,
    pos.y };
  drawData->AddText( labelPos, gStyle.fontSize, label, gStyle.textColor, &clipRect );


  // TODO:
  //   it seems dumb that we need to get the mouse position twice.
  //   since IsHovered() already got the cursor position and checked
  //   for errors
  if( gTacImGuiGlobals.IsHovered( clipRect ) )
  {
    float mousePositionTextSpace = gTacImGuiGlobals.mMousePositionDesktopWindowspace.x - textPos.x;
    int numGlyphsBeforeCaret = TacGetCaret( drawData, codepoints, mousePositionTextSpace );
    if( keyboardInput->mCurr.IsKeyDown( TacKey::MouseLeft ) )
    {
      if( keyboardInput->mPrev.IsKeyDown( TacKey::MouseLeft ) )
        inputData.OnDrag( numGlyphsBeforeCaret );
      else
        inputData.OnClick( numGlyphsBeforeCaret );
    }
  }

  if( inputData.mCaretCount == 1 )
  {
    float caretPos = drawData->CalculateTextSize(
      inputData.mCodepoints.data(),
      inputData.mNumGlyphsBeforeCaret[ 0 ],
      gStyle.fontSize ).x;
    float caretYPadding = 2.0f;
    float caretHalfWidth = 0.5f;
    v2 caretMini = {
      textPos.x + caretPos - caretHalfWidth,
      textPos.y + caretYPadding };
    v2 caretMaxi = {
      textPos.x + caretPos + caretHalfWidth,
      textPos.y + totalSize.y - caretYPadding };
    float caretColorAlpha = ( ( std::sin( 6.0f * ( float )gTacImGuiGlobals.mElapsedSeconds ) + 1.0f ) / 2.0f );
    v4 caretColor = { 0, 0, 0, caretColorAlpha };
    drawData->AddBox( caretMini, caretMaxi, caretColor, nullptr, &clipRect );
  }


  if( codepoints.size() != inputData.mCodepoints.size() )
  {
    TacString newText;
    TacUTF8Converter::Convert( inputData.mCodepoints, newText );
    text = newText;
    textChanged = true;
  }

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
    if( keyboardInput->IsKeyJustDown( TacKey::MouseLeft ) )
    {
      color3 /= 2.0f;
      clicked = true;
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
    if( keyboardInput->IsKeyDown( TacKey::MouseLeft ) )
    {
      outerBoxColor /= 2.0f;
    }
    if( keyboardInput->IsKeyJustDown( TacKey::MouseLeft ) )
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
  textSize.y;

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
    if( keyboardInput->IsKeyDown( TacKey::MouseLeft ) )
    {
      outerBoxColor = v4( 0.3f, 0.3f, 0, 1 );
    }
    if( keyboardInput->IsKeyJustDown( TacKey::MouseLeft ) )
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
      TacDefaultVertex2D* defaultVertex2D = &drawData->mDefaultVertex2Ds[ iVert ];
      defaultVertex2D->mPosition = p0;
      defaultVertex2D++;
      defaultVertex2D->mPosition = p1;
      defaultVertex2D++;
      defaultVertex2D->mPosition = p2;
      defaultVertex2D++;
      defaultVertex2D->mPosition = p3;

      CBufferPerObject perObjectData = {};
      perObjectData.World = m4::Identity();
      perObjectData.Color = checkmarkColor;

      TacUI2DDrawCall drawCall;
      drawCall.mIIndexCount = 6;
      drawCall.mIIndexStart = iIndex;
      drawCall.mIVertexCount = 4;
      drawCall.mIVertexStart = iVert;
      drawCall.mShader = drawData->mUI2DCommonData->mShader;
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
