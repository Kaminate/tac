#include "common/tacImGui.h"
#include "common/math/tacVector4.h"
#include "common/math/tacMath.h"
#include "common/tacUI.h"
#include "common/tacUI2D.h"
#include "common/tackeyboardinput.h"
#include "common/tacDesktopWindow.h"
#include "common/tacOS.h"
#include "common/tacTextEdit.h"

//TacUIRoot* mUIRoot = nullptr;


struct TacUIStyle
{
  float windowPadding = 8;
  v2 itemSpacing = { 8, 4 };

  // Should this be a float?
  int fontSize = 16;

  float buttonPadding = 3.0f;
  v4 textColor = { 1, 1, 0, 1 };
} gStyle;

TacVector< TacImGuiWindow* > gTacImGuiWindows;
static TacImGuiWindow* TacImGuiFindWindow( const TacString& name )
{
  for( TacImGuiWindow* window : gTacImGuiWindows )
  {
    if( window->mName == name )
      return window;
  }
  return nullptr;
}

TacImGuiWindow* TacImGuiWindow::BeginChild( const TacString& name, v2 size )
{
  TacImGuiWindow* child = TacImGuiFindWindow( name );
  if( !child )
  {
    child = new TacImGuiWindow;
    child->mName = name;
    child->mParent = this;
    child->mUIRoot = mUIRoot;
    gTacImGuiWindows.push_back( child );
  }

  child->mSize =
  {
    size.x > 0 ? size.x : mSize.x + size.x,
    size.y > 0 ? size.y : mSize.y + size.y
  };

  child->BeginFrame();
  return child;
}
void TacImGuiWindow::EndChild()
{
  TacAssert( mParent );
  mParent->ItemSize( mSize );
}
void TacImGuiWindow::BeginFrame()
{
  TacUI2DDrawData* ui2DDrawData = mUIRoot->mUI2DDrawData;
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

      if( !mUIRoot->mKeyboardInput->IsKeyDown( TacKey::MouseLeft ) )
        mScrolling = false;
    }
    else if( mUIRoot->mKeyboardInput->IsKeyJustDown( TacKey::MouseLeft ) &&
      IsHovered( TacImGuiRect::FromMinMax( mini, maxi ) ) )
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

void TacImGuiWindow::Checkbox( const TacString& str, bool* value )
{
  v2 pos = mCurrCursorDrawPos;

  v2 textSize = mUIRoot->mUI2DDrawData->CalculateTextSize( str, gStyle.fontSize );
  textSize.y;

  float boxWidth = textSize.y;
  v2 boxSize = v2( 1, 1 ) * boxWidth;

  v2 totalSize = v2( boxWidth + gStyle.itemSpacing.x + textSize.x, textSize.y );
  ItemSize( totalSize );

  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, totalSize );
  ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return;

  bool hovered = IsHovered( clipRect );

  // TODO: move to a drawData->rect( topleft, bottomright, rounding )
  TacUI2DDrawData* mUI2DDrawData = mUIRoot->mUI2DDrawData;
  TacUI2DCommonData* mUI2DCommonData = mUI2DDrawData->mUI2DCommonData;

  v4 outerBoxColor = v4( 1, 1, 0, 1 );
  if( hovered )
  {
    outerBoxColor = v4( 0.5f, 0.5f, 0, 1 );
    if( mUIRoot->mKeyboardInput->IsKeyDown( TacKey::MouseLeft ) )
    {
      outerBoxColor = v4( 0.3f, 0.3f, 0, 1 );
    }
    if( mUIRoot->mKeyboardInput->IsKeyJustDown( TacKey::MouseLeft ) )
    {
      *value = !*value;
    }
  }

  mUI2DDrawData->AddBox( pos, pos + boxSize, outerBoxColor, nullptr, &clipRect );
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
      int iVert = mUI2DDrawData->mDefaultVertex2Ds.size();
      int iIndex = mUI2DDrawData->mDefaultIndex2Ds.size();
      mUI2DDrawData->mDefaultIndex2Ds.push_back( iVert + 0 );
      mUI2DDrawData->mDefaultIndex2Ds.push_back( iVert + 1 );
      mUI2DDrawData->mDefaultIndex2Ds.push_back( iVert + 2 );
      mUI2DDrawData->mDefaultIndex2Ds.push_back( iVert + 1 );
      mUI2DDrawData->mDefaultIndex2Ds.push_back( iVert + 3 );
      mUI2DDrawData->mDefaultIndex2Ds.push_back( iVert + 2 );
      mUI2DDrawData->mDefaultVertex2Ds.resize( iVert + 4 );
      TacDefaultVertex2D* defaultVertex2D = &mUI2DDrawData->mDefaultVertex2Ds[ iVert ];
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
      drawCall.mShader = mUI2DCommonData->m2DTextShader;
      drawCall.mUniformSource = TacTemporaryMemory( perObjectData );
      mUI2DDrawData->mDrawCall2Ds.push_back( drawCall );
    }
    else
    {
      v2 innerPadding = v2( 1, 1 ) * 3;
      mUI2DDrawData->AddBox( pos + innerPadding, pos + boxSize - innerPadding, checkmarkColor, nullptr, &clipRect );
    }
  }

  v2 textPos = {
    pos.x + boxWidth + gStyle.itemSpacing.x,
    pos.y };
  mUI2DDrawData->AddText( textPos, gStyle.fontSize, str, gStyle.textColor, &clipRect );
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

bool TacImGuiWindow::IsHovered( const TacImGuiRect& rect )
{
  if( !mUIRoot->mDesktopWindow->mCursorUnobscured )
    return false;

  TacErrors errors;
  v2 mousePosScreenspace = {};
  TacOS::Instance->GetScreenspaceCursorPos( mousePosScreenspace, errors );
  if( errors.size() )
    return false;

  v2 mousePosWindowspace = mousePosScreenspace - v2(
    ( float )mUIRoot->mDesktopWindow->mX,
    ( float )mUIRoot->mDesktopWindow->mY );
  return
    mousePosWindowspace.x > rect.mMini.x &&
    mousePosWindowspace.x < rect.mMaxi.x &&
    mousePosWindowspace.y > rect.mMini.y &&
    mousePosWindowspace.y < rect.mMaxi.y;
}

bool TacImGuiWindow::Button( const TacString& str )
{
  bool justClicked = false;
  v2 textSize = mUIRoot->mUI2DDrawData->CalculateTextSize( str, gStyle.fontSize );
  v2 buttonSize = { textSize.x + 2 * gStyle.buttonPadding, textSize.y };
  v2 pos = mCurrCursorDrawPos;
  ItemSize( textSize );


  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, buttonSize );
  ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return justClicked;

  bool hovered = IsHovered( clipRect );

  v3 outerBoxColor = v3( .23f, .28f, .38f );
  if( hovered )
  {
    outerBoxColor /= 2.0f;
    if( mUIRoot->mKeyboardInput->IsKeyDown( TacKey::MouseLeft ) )
    {
      outerBoxColor /= 2.0f;
    }
    if( mUIRoot->mKeyboardInput->IsKeyJustDown( TacKey::MouseLeft ) )
    {
      justClicked = true;
    }
  }

  mUIRoot->mUI2DDrawData->AddBox( pos, pos + buttonSize, v4( outerBoxColor, 1 ), nullptr, &clipRect );

  v2 textPos = {
    pos.x + gStyle.buttonPadding,
    pos.y };
  mUIRoot->mUI2DDrawData->AddText( textPos, gStyle.fontSize, str, gStyle.textColor, &clipRect );

  return justClicked;
}

bool TacImGuiWindow::Selectable( const TacString& str, bool selected )
{
  bool clicked = false;
  v2 pos = mCurrCursorDrawPos;
  v2 buttonSize = {
    mContentRect.mMaxi.x - pos.x,
    ( float )gStyle.fontSize };

  ItemSize( buttonSize );

  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, buttonSize );
  ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return clicked;

  v3 color3 = v3( 0.7f, 0.3f, 0.3f ) * 0.7f;
  if( selected )
    color3 = ( color3 + v3( 1, 1, 1 ) ) * 0.3f;


  bool hovered = IsHovered( clipRect );
  if( hovered )
  {
    color3 /= 2.0f;
    if( mUIRoot->mKeyboardInput->IsKeyJustDown( TacKey::MouseLeft ) )
    {
      color3 /= 2.0f;
      clicked = true;
    }
  }

  v4 color( color3, 1 );

  mUIRoot->mUI2DDrawData->AddBox( pos, pos + buttonSize, color, nullptr, &clipRect );
  mUIRoot->mUI2DDrawData->AddText( pos, gStyle.fontSize, str, gStyle.textColor, &clipRect );
  return clicked;
}

void TacImGuiWindow::SameLine()
{
  mCurrCursorDrawPos = {
    mPrevCursorDrawPos.x + gStyle.itemSpacing.x,
    mPrevCursorDrawPos.y };
  mCurrLineHeight = mPrevLineHeight;
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

void TacImGuiWindow::BeginGroup()
{
  mGroupSavedCursorDrawPos = mCurrCursorDrawPos;
  mXOffsets.push_back( mCurrCursorDrawPos.x - mPos.x );
  // restore on end group?
  mCurrLineHeight = 0;
}
void TacImGuiWindow::EndGroup()
{
  mXOffsets.pop_back();
  v2 groupEndPos = {
    mMaxiCursorDrawPos.x,
    mMaxiCursorDrawPos.y + mPrevLineHeight };
  v2 groupSize = groupEndPos - mGroupSavedCursorDrawPos;
  mCurrCursorDrawPos = mGroupSavedCursorDrawPos;
  ItemSize( groupSize );
}

void TacImGuiWindow::UpdateMaxCursorDrawPos( v2 pos )
{
  mMaxiCursorDrawPos.x = TacMax( mMaxiCursorDrawPos.x, pos.x );
  mMaxiCursorDrawPos.y = TacMax( mMaxiCursorDrawPos.y, pos.y );
}

void TacImGuiWindow::Text( const TacString& utf8 )
{
  //auto state = mUIRoot->mUI2DDrawData->PushState();
  //state->Draw2DText(...);

  v2 textPos = mCurrCursorDrawPos;
  v2 textSize = mUIRoot->mUI2DDrawData->CalculateTextSize( utf8, gStyle.fontSize );


  ItemSize( textSize );

  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( textPos, textSize );
  ComputeClipInfo( &clipped, &clipRect );
  if( clipped )
    return;

  mUIRoot->mUI2DDrawData->AddText( textPos, gStyle.fontSize, utf8, gStyle.textColor, &clipRect );
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

bool TacImGuiWindow::InputText( const TacString& label, TacString& text )
{
  TacErrors cursorErrors;
  static TacTextInputData inputData;
  v2 mousePositionScreenspace;
  TacOS::Instance->GetScreenspaceCursorPos( mousePositionScreenspace, cursorErrors );

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

  TacUI2DDrawData* drawData = mUIRoot->mUI2DDrawData;
  TacKeyboardInput* keyboardInput = mUIRoot->mKeyboardInput;

  v2 pos = mCurrCursorDrawPos;

  // Word wrap?
  int lineCount = 1;
  for( char c : text )
    if( c == '\n' )
      lineCount++;

  v2 totalSize = {
    mContentRect.mMaxi.x - pos.x,
    ( float )lineCount * ( float )gStyle.fontSize };

  ItemSize( totalSize );

  bool clipped;
  auto clipRect = TacImGuiRect::FromPosSize( pos, totalSize );
  ComputeClipInfo( &clipped, &clipRect );
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
  static v2 lastMouseReleasePositionScreenspace;
  if( keyboardInput->HasKeyJustBeenReleased( TacKey::MouseLeft ) &&
    cursorErrors.empty() &&
    !inputData.mCodepoints.empty() )
  {
    auto mouseReleaseSeconds = mUIRoot->GetElapsedSeconds();
    if( mouseReleaseSeconds - lastMouseReleaseSeconds < 0.5f &&
      lastMouseReleasePositionScreenspace == mousePositionScreenspace )
    {
      inputData.mNumGlyphsBeforeCaret[ 0 ] = 0;
      inputData.mNumGlyphsBeforeCaret[ 1 ] = inputData.mCodepoints.size();
      inputData.mCaretCount = 2;
    }
    lastMouseReleaseSeconds = mouseReleaseSeconds;
    lastMouseReleasePositionScreenspace = mousePositionScreenspace;
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
  if( IsHovered( clipRect ) && cursorErrors.empty() )
  {
    v2 mousePositionWindowspace = mousePositionScreenspace - v2(
      ( float )mUIRoot->mDesktopWindow->mX,
      ( float )mUIRoot->mDesktopWindow->mY );
    float mousePositionTextSpace = mousePositionWindowspace.x - textPos.x;
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
    float caretColorAlpha = ( ( std::sin( 6.0f * ( float )*mUIRoot->mElapsedSeconds ) + 1.0f ) / 2.0f );
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
