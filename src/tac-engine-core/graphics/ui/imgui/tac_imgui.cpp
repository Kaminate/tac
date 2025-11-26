#include "tac_imgui.h" // self-inc

#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_drag.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h"
#include "tac-engine-core/graphics/ui/tac_text_edit.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"

#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/window/tac_app_window_api.h"

#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_frame_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string.h"

namespace Tac
{
  static int ComputeLineCount( const StringView& s )
  {
    // todo: word wrap
    int lineCount{ 1 };
    for( char c : s )
      if( c == '\n' )
        lineCount++;
    return lineCount;
  }

  static int GetCaret( const Vector< Codepoint >& codepoints,
                       float mousePos ) // mouse pos rel text top left corner
  {
    const int codepointCount{ codepoints.size() };

    float runningTextWidth{};
    int numGlyphsBeforeCaret{};
    for( int i{ 1 }; i <= codepointCount; ++i )
    {
      const v2 substringSize{ CalculateTextSize( codepoints.begin(), i, ImGuiGetFontSize() ) };
      const float lastGlyphWidth{ substringSize.x - runningTextWidth };
      const float lastGlyphMidpoint{ runningTextWidth + lastGlyphWidth / 2 };

      if( mousePos < lastGlyphMidpoint )
        break;

      runningTextWidth += lastGlyphWidth;
      numGlyphsBeforeCaret++;
    }
    return numGlyphsBeforeCaret;
  }


  static SettingsNode ImGuiGetWindowSettingsJson( const StringView& name )
  {
    SettingsNode windowsJson{ ImGuiGlobals::Instance.mSettingsNode.GetChild( "imgui.windows" ) };
    const int n{ windowsJson.GetValue().mArrayElements.size() };
    for( int i{}; i < n; ++i )
    {
      SettingsNode child{ windowsJson.GetChild( "[" + ToString( i ) + "]" ) };
      const StringView childName{ ( StringView )child.GetChild( "name" ).GetValue().mString };
      if( childName == name )
        return child;
    }

    SettingsNode child{ windowsJson.GetChild( "[" + ToString( n ) + "]" ) };
    child.GetChild( "name" ).SetValue( name );
    return child;
  }

  static void ImGuiSaveWindowWithSettings( const StringView& name, int x, int y, int w, int h )
  {
    SettingsNode windowJson{ ImGuiGetWindowSettingsJson( name ) };
    windowJson.GetChild( "name" ).SetValue( name );
    windowJson.GetChild( "x" ).SetValue( ( JsonNumber )x );
    windowJson.GetChild( "y" ).SetValue( ( JsonNumber )y );
    windowJson.GetChild( "w" ).SetValue( ( JsonNumber )w );
    windowJson.GetChild( "h" ).SetValue( ( JsonNumber )h );
  }

  static void ImGuiSaveWindowSettings( ImGuiWindow* window )
  {
    ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    globals.mSettingsDirty = false;
    
    //if( !window->mWindowHandleOwned )
    //  return;

    WindowHandle h{ window->GetWindowHandle() };
    if( !AppWindowApi::IsShown( h ) )
      return;

    const v2i pos{ AppWindowApi::GetPos( h ) };
    const v2i size{ AppWindowApi::GetSize( h ) };
    ImGuiSaveWindowWithSettings( window->mName, pos.x, pos.y, size.x, size.y );
  }

  static const v4& GetFrameColor( const bool hovered )
  {
    //ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    const bool active{ hovered && AppKeyboardApi::IsPressed( Key::MouseLeft ) };
    const v4& boxColor{ ImGuiGetColor( active
                                       ? ImGuiCol::FrameBGActive : hovered
                                       ? ImGuiCol::FrameBGHovered : ImGuiCol::FrameBG ) };
    return boxColor;
  }

  static void ImGuiDeleteWindows()
  {
    ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    
    dynmc int nAllWindows{ globals.mAllWindows.size() };
    dynmc int iAllWindows{};
    while( iAllWindows < nAllWindows )
    {
      ImGuiWindow* window{ globals.mAllWindows[ iAllWindows ] };
      const GameTime curSeconds{ globals.mElapsedSeconds };
      const GameTimeDelta deletionWaitSeconds{ 0.1f };
      if( curSeconds > window->mRequestTime + deletionWaitSeconds )
      {
        if( window->mWindowHandleOwned )
        {
          AppWindowApi::DestroyWindow( window->GetWindowHandle() );
        }

        globals.mAllWindows[ iAllWindows ] = globals.mAllWindows[ --nAllWindows ];
        TAC_DELETE window;
      }
      else
      {
        ++iAllWindows;
      }
    }
    globals.mAllWindows.resize( nAllWindows );
  }

  // -----------------------------------------------------------------------------------------------

  UIStyle::UIStyle()
  {
    ImGuiDefaultColors();
  }

  // -----------------------------------------------------------------------------------------------

  auto ImGuiRect::FromPosSize( v2 pos, v2 size ) -> ImGuiRect { return { .mMini { pos }, .mMaxi { pos + size }, }; }
  auto ImGuiRect::FromMinMax( v2 mini, v2 maxi ) -> ImGuiRect { return { .mMini { mini }, .mMaxi { maxi }, }; }
  auto ImGuiRect::GetWidth() const -> float { return mMaxi.x - mMini.x; }
  auto ImGuiRect::GetHeight() const -> float { return mMaxi.y - mMini.y; }
  auto ImGuiRect::GetSize() const -> v2 { return mMaxi - mMini; }
  bool ImGuiRect::ContainsPoint( v2 p ) const
  {
    return
      p.x > mMini.x &&
      p.x < mMaxi.x &&
      p.y > mMini.y &&
      p.y < mMaxi.y;
  }

  bool ImGuiRect::Contains( const ImGuiRect& r ) const
  {
    return
      r.mMini.x >= mMini.x &&
      r.mMaxi.x <= mMaxi.x &&
      r.mMini.y >= mMini.y &&
      r.mMaxi.y <= mMaxi.y;
  }

  bool ImGuiRect::Overlaps( const ImGuiRect& r ) const
  {
    return
      r.mMini.x <= mMaxi.x &&
      r.mMaxi.x >= mMini.x &&
      r.mMini.y <= mMaxi.y &&
      r.mMaxi.y >= mMini.y;
  }

  // -----------------------------------------------------------------------------------------------

  static void ImGuiDrawCheckMark( const v2& pos, const  float boxWidth )
  {
    ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
    UI2DDrawData* drawData{ window->mDrawData };

    const v4& checkmarkColor{ ImGuiGetColor( ImGuiCol::Checkmark ) };

    // (0,0)-------+
    // |         2 |
    // |        /  |
    // | 0     /   |
    // |   \  /    |
    // |     1     |
    // +-------(1,1)
    v2 ps[]{ v2( 0.2f, 0.4f ), v2( 0.45f, 0.90f ), v2( 0.9f, 0.1f ) };
    for( v2& p : ps )
      p = p * boxWidth + pos;

    const float lineRadius{ boxWidth * 0.05f };

    const UI2DDrawData::Line firstLine
    {
      .mP0         { ps[ 0 ] },
      .mP1         { ps[ 1 ] },
      .mLineRadius { lineRadius },
      .mColor      { checkmarkColor },
    };

    const UI2DDrawData::Line secondLine
    {
      .mP0         { ps[ 1 ] },
      .mP1         { ps[ 2 ] },
      .mLineRadius { lineRadius },
      .mColor      { checkmarkColor },
    };

    drawData->PushDebugGroup( "Checkmark" );
    drawData->AddLine( firstLine );
    drawData->AddLine( secondLine );
    drawData->PopDebugGroup();
  }

  // -----------------------------------------------------------------------------------------------

  static Vector< UI2DDrawGpuInterface > sDrawInterfaces;
}

void Tac::TextInputDataUpdateKeys( TextInputData* inputData, const v2& mousePos, const v2& textPos )
{
  struct KeyMap
  {
    Key           mKey;
    TextInputKey  mTextInputKey;
  };
  const KeyMap keyMaps[]
  {
    { Key::LeftArrow, TextInputKey::LeftArrow },
    { Key::RightArrow, TextInputKey::RightArrow },
    { Key::Backspace, TextInputKey::Backspace },
    { Key::Delete, TextInputKey::Delete },
  };

  for( const KeyMap& keyMap : keyMaps )
    if( AppKeyboardApi::JustPressed( keyMap.mKey ) )
      inputData->OnKeyPressed( keyMap.mTextInputKey );

  const CodepointView codepoints{ AppKeyboardApi::GetCodepoints() };
  for( Codepoint codepoint : codepoints )
    inputData->OnCodepoint( codepoint );

  if( AppKeyboardApi::JustPressed( Key::MouseLeft ) )
  {
    const int numGlyphsBeforeCaret = GetCaret( inputData->mCodepoints, mousePos.x - textPos.x );
    inputData->OnClick( numGlyphsBeforeCaret );
  }
  else if( AppKeyboardApi::IsPressed( Key::MouseLeft ) )
  {
    const int numGlyphsBeforeCaret = GetCaret( inputData->mCodepoints, mousePos.x - textPos.x );
    inputData->OnDrag( numGlyphsBeforeCaret );
  }
}

void Tac::TextInputDataDrawSelection( TextInputData* inputData,
                                      UI2DDrawData* drawData,
                                      const v2& textPos,
                                      const ImGuiRect* clipRect )
{
  const float fontSize{ ImGuiGetFontSize() };
  const Codepoint* codepoints{ inputData->mCodepoints.data() };
  if( inputData->mCaretCount == 2 )
  {
    const int minCaret{ inputData->GetMinCaret() };
    const int maxCaret{ inputData->GetMaxCaret() };
    const float minCaretPos{ CalculateTextSize( codepoints, minCaret, fontSize ).x };
    const float maxCaretPos{ CalculateTextSize( codepoints, maxCaret, fontSize ).x };
    drawData->AddBox(
      UI2DDrawData::Box
      {
        .mMini  { textPos + v2( minCaretPos, 0 ) },
        .mMaxi  { textPos + v2( maxCaretPos, fontSize ) },
        .mColor { ImGuiGetColor( ImGuiCol::TextSelection ) },
      }, clipRect );
  }

  if( inputData->mCaretCount == 1 )
  {
    const int codepointCount{ inputData->mNumGlyphsBeforeCaret[ 0 ] };
    const float caretPos{ CalculateTextSize( codepoints, codepointCount, fontSize ).x };
    const float caretYPadding{ 2.0f };
    const float caretHalfWidth{ 0.5f };
    const float blinkySpeed{ 2.25f };
    const double t{ ImGuiGlobals::Instance.mElapsedSeconds };
    const float a{ 1 - Pow( ( float )Cos( blinkySpeed * t ), 4 ) };
    drawData->AddBox(
      UI2DDrawData::Box
      {
        .mMini { textPos + v2( caretPos - caretHalfWidth, caretYPadding ) },
        .mMaxi { textPos + v2( caretPos + caretHalfWidth, fontSize - caretYPadding ) },
        .mColor { v4( ImGuiGetColor( ImGuiCol::Text ).xyz(), a ) },
      }, clipRect );
  }
}

auto Tac::ImGuiGetMouseCursor() -> Tac::ImGuiMouseCursor
{
  return ImGuiGlobals::Instance.mMouseCursor;
}

auto Tac::ImGuiGetColName( const ImGuiCol colIdx ) -> const char*
{
  constexpr const char* names[]
  {
    "Text",
    "TextSelection",
    "WindowBackground",
    "ChildWindowBackground",
    "FrameBG",
    "FrameBGHovered",
    "FrameBGActive",
    "Scrollbar",
    "ScrollbarHovered",
    "ScrollbarActive",
    "ScrollbarBG",
    "Checkmark",
  };
  static_assert( TAC_ARRAY_SIZE( names ) == ( int )ImGuiCol::Count );
  return names[ ( int )colIdx ];
}

// -----------------------------------------------------------------------------------------------

void Tac::ImGuiSetNextWindowStretch()
{
  ImGuiNextWindow::gNextWindow.mStretch = true;
}

void Tac::ImGuiSetNextWindowHandle( const WindowHandle& WindowHandle )
{
  ImGuiNextWindow::gNextWindow.mWindowHandle = WindowHandle;
}

auto Tac::ImGuiGetWindowHandle() -> WindowHandle
{
  return ImGuiGlobals::Instance.mCurrentWindow->GetWindowHandle();
}

auto Tac::ImGuiGetWindowHandle( StringView name ) -> WindowHandle
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.FindWindow( name ) };
  return window ? window->mDesktopWindow->mWindowHandle : WindowHandle{};
}

auto Tac::ImGuiGetContentRect() -> Tac::ImGuiRect
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  return window->mViewportSpaceVisibleRegion;
}

void Tac::ImGuiSetNextWindowMoveResize()            { ImGuiNextWindow::gNextWindow.mMoveResize = true; }
void Tac::ImGuiSetNextWindowPosition( v2 position, ImGuiCondition cond )
{
  TAC_UNUSED_PARAMETER( cond );
  ImGuiNextWindow::gNextWindow.mPosition = position;
  ImGuiNextWindow::gNextWindow.mPositionValid = true;
}
void Tac::ImGuiSetNextWindowSize( v2 size, ImGuiCondition cond ) 
{
  TAC_UNUSED_PARAMETER( cond );
  ImGuiNextWindow::gNextWindow.mSize = size;
  ImGuiNextWindow::gNextWindow.mSizeValid = true;
}
void Tac::ImGuiSetNextWindowDisableBG() { ImGuiNextWindow::gNextWindow.mEnableBG = false; }

auto Tac::GetID( StringView label ) -> Tac::ImGuiID
{
  return ImGuiGlobals::Instance.mCurrentWindow->GetID( label );
}

void Tac::PushID( StringView str )
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  const ImGuiID id{ window->GetID( str ) };
  window->mIDStack.push_back( id );
}

void Tac::PopID()
{
  ImGuiGlobals::Instance.mCurrentWindow->mIDStack.pop_back();
}


// In Dear ImGui, imgui.begin must be followed by imgui.end, regardless of imgui.begin return val
// In Tac ImGui, this is not the case.
bool Tac::ImGuiBegin( const StringView& name )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  
  //ImGuiCreateWindow createWindowFn = globals.mCreateWindow;
  ImGuiWindow* window{ globals.FindWindow( name ) };
  if( !window )
  {
    WindowHandle hDesktopWindow{ ImGuiNextWindow::gNextWindow.mWindowHandle };
    int desktopWindowWidth{};
    int desktopWindowHeight{};

    if( hDesktopWindow.IsValid() )
    {
      if( !AppWindowApi::IsShown( hDesktopWindow ) )
        return false;

      const v2i size{ AppWindowApi::GetSize( hDesktopWindow ) };
      desktopWindowWidth = size.x;
      desktopWindowHeight = size.y;
    }
    else
    {
      int x{ 50 };
      int y{ 50 };
      int w{ 800 };
      int h{ 600 };

      if( ImGuiNextWindow::gNextWindow.mPositionValid )
      {
        x = ( int )ImGuiNextWindow::gNextWindow.mPosition.x;
        y = ( int )ImGuiNextWindow::gNextWindow.mPosition.y;
      }

      if( ImGuiNextWindow::gNextWindow.mSizeValid )
      {
        w = ( int )ImGuiNextWindow::gNextWindow.mSize.x;
        h = ( int )ImGuiNextWindow::gNextWindow.mSize.y;
      }

      SettingsNode windowJson{ ImGuiGetWindowSettingsJson( name ) };
      x = ( int )windowJson.GetChild( "x" ).GetValueWithFallback( ( JsonNumber )x ).mNumber;
      y = ( int )windowJson.GetChild( "y" ).GetValueWithFallback( ( JsonNumber )y ).mNumber;
      w = ( int )windowJson.GetChild( "w" ).GetValueWithFallback( ( JsonNumber )w ).mNumber;
      h = ( int )windowJson.GetChild( "h" ).GetValueWithFallback( ( JsonNumber )h ).mNumber;

      //Platform* platform = Platform::GetInstance();
      //

      //const WindowApi::CreateParams createParams
      //{
      //  .mName = name,
      //  .mX = x,
      //  .mY = y,
      //  .mWidth = w,
      //  .mHeight = h,
      //};

      const v2i pos{ x, y };
      const v2i size{ w, h };
      const WindowCreateParams params
      {
        .mName { name },
        .mPos  { pos },
        .mSize { size }
      };

      Errors errors;
      hDesktopWindow = AppWindowApi::CreateWindow( params, errors );
      TAC_ASSERT_MSG( !errors, errors.ToString() );

      desktopWindowWidth = w;
      desktopWindowHeight = h;
    }

    const v2 size{ ImGuiNextWindow::gNextWindow.mSize.x + ( ImGuiNextWindow::gNextWindow.mSize.x > 0 ? 0 : desktopWindowWidth ),
                   ImGuiNextWindow::gNextWindow.mSize.y + ( ImGuiNextWindow::gNextWindow.mSize.y > 0 ? 0 : desktopWindowHeight ) };

    ImGuiDesktopWindowImpl* imguiDesktopWindow
      = ImGuiGlobals::Instance.FindDesktopWindow( hDesktopWindow );
    if( !imguiDesktopWindow )
    {
      imguiDesktopWindow = TAC_NEW ImGuiDesktopWindowImpl;
      imguiDesktopWindow->mWindowHandle = hDesktopWindow;
      ImGuiGlobals::Instance.mDesktopWindows.push_back( imguiDesktopWindow );
    }

    TAC_ASSERT( hDesktopWindow.IsValid() );
    window = TAC_NEW ImGuiWindow;
    window->mName = name;
    window->mDrawData = TAC_NEW UI2DDrawData;
    window->mTextInputData = TAC_NEW TextInputData;
    window->mDesktopWindow = imguiDesktopWindow;
    window->mWindowHandleOwned = !ImGuiNextWindow::gNextWindow.mWindowHandle.IsValid();
    window->mViewportSpacePos = {};
    window->mSize = size;
    window->mStretchWindow = ImGuiNextWindow::gNextWindow.mStretch;
    window->mMoveResizeWindow = ImGuiNextWindow::gNextWindow.mMoveResize;
    window->mEnableBG = ImGuiNextWindow::gNextWindow.mEnableBG;
    ImGuiGlobals::Instance.mAllWindows.push_back( window );
  }


  ImGuiNextWindow::gNextWindow = {};

  TAC_ASSERT( window->mSize.x > 0 && window->mSize.y > 0 );

  ImGuiDesktopWindow* desktopWindow{ window->mDesktopWindow };
  const WindowHandle hWindow { desktopWindow->mWindowHandle };
  window->mRequestTime = ImGuiGlobals::Instance.mElapsedSeconds;
  if( !AppWindowApi::IsShown( hWindow ) )
    return false;

  ImGuiGlobals::Instance.mWindowStack.push_back( window );
  ImGuiGlobals::Instance.mCurrentWindow = window;

  if( window->mStretchWindow )
    window->mSize = AppWindowApi::GetSize( hWindow );

  window->BeginFrame();

  return true;
}

void Tac::ImGuiEnd()
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  Vector< ImGuiWindow* >& windowStack{ globals.mWindowStack };
  ImGuiWindow* backWindow{ windowStack.back() };
  backWindow->EndFrame();
  windowStack.pop_back();
  globals.mCurrentWindow = windowStack.empty() ? nullptr : windowStack.back();
}

void Tac::ImGuiBeginChild( const StringView& name, const v2& size )
{
  ImGuiGlobals& Instance{ ImGuiGlobals::Instance };
  ImGuiWindow* parent{ Instance.mCurrentWindow };
  ImGuiWindow* child{ Instance.FindWindow( name ) };
  if( !child )
  {
    child = TAC_NEW ImGuiWindow;
    child->mName = name;
    child->mParent = parent;
    child->mDrawData = parent->mDrawData;
    child->mTextInputData = parent->mTextInputData;
    Instance.mAllWindows.push_back( child );
  }
  child->mRequestTime = ImGuiGlobals::Instance.mElapsedSeconds;
  child->mSize = v2( size.x > 0 ? size.x : size.x + parent->mSize.x,
                     size.y > 0 ? size.y : size.y + parent->mSize.y );
  Instance.mWindowStack.push_back( child );
  Instance.mCurrentWindow = child;
  child->BeginFrame();
}

void Tac::ImGuiEndChild()
{
  ImGuiGlobals& Instance{ ImGuiGlobals::Instance };
  ImGuiWindow* child{ Instance.mCurrentWindow };
  child->mParent->ItemSize( child->mSize );
  child->EndFrame();
  Instance.mWindowStack.pop_back();
  Instance.mCurrentWindow = Instance.mWindowStack.back();
}

void Tac::ImGuiBeginGroup()
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  window->PushXOffset();
  window->mCurrLineHeight = 0;
  window->mGroupSK.push_back(
    GroupData
    {
      .mSavedCursorDrawPos { window->mViewportSpaceCurrCursor },
      .mSavedLineHeight    { window->mCurrLineHeight },
    } );
}

void Tac::ImGuiEndGroup()
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  const GroupData& groupData{ window->mGroupSK.back() };
  const v2 groupSize{ window->mViewportSpaceMaxiCursor - groupData.mSavedCursorDrawPos };

  window->mXOffsets.pop_back();
  window->mCurrLineHeight = groupData.mSavedLineHeight;
  window->mViewportSpaceCurrCursor = groupData.mSavedCursorDrawPos;
  window->ItemSize( groupSize );
  window->mGroupSK.pop_back();
}

void Tac::ImGuiIndent()
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  window->mViewportSpaceCurrCursor.x += 15.0f;
  window->PushXOffset();
}

void Tac::ImGuiUnindent()
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  window->mXOffsets.pop_back();
  window->mViewportSpaceCurrCursor.x = window->mViewportSpacePos.x + window->mXOffsets.back();
}

void Tac::ImGuiSameLine()
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  window->mViewportSpaceCurrCursor
    = window->mViewportSpacePrevCursor
    + v2( ImGuiGetStyle().itemSpacing.x, 0 );
  window->mCurrLineHeight = window->mPrevLineHeight;
}

void Tac::ImGuiText( const StringView& utf8 )
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  UI2DDrawData* drawData{ window->mDrawData };
  const v2 textPos{ window->mViewportSpaceCurrCursor };
  const v2 textSize{ CalculateTextSize( utf8, ImGuiGetFontSize() ) };
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( textPos, textSize ) };
  window->ItemSize( textSize );
  if( !window->Overlaps( origRect ) )
    return;

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  drawData->PushDebugGroup( ShortFixedString::Concat( "ImGuiText(", utf8, ")" ) );
  drawData->AddText(
    UI2DDrawData::Text
    {
      .mPos      { textPos },
      .mFontSize { ImGuiGetFontSize() },
      .mUtf8     { utf8 },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &clipRect );
  drawData->PopDebugGroup();
}

bool Tac::ImGuiInputText( const StringView& label, String& text )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  const float buttonPadding{ ImGuiGetButtonPadding() };
  const v2& itemSpacing{ ImGuiGetItemSpacing() };

  const GameTime mouseReleaseSeconds{ globals.mElapsedSeconds };

  ImGuiWindow* window{ globals.mCurrentWindow };

  const ImGuiID id{ window->GetID( label ) };
  const String oldText{ text };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  const int lineCount{ ComputeLineCount( text ) };
  const float width { window->GetRemainingWidth() };
  const v2 totalSize( width, lineCount * ImGuiGetFontSize() );
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, totalSize ) };

  window->ItemSize( totalSize );
  if( !window->Overlaps( origRect ) )
    return false;


  TextInputData* textInputData{ window->mTextInputData };

  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( ShortFixedString::Concat( "ImGuiInputText(" , label , ")" ) );
  TAC_ON_DESTRUCT( drawData->PopDebugGroup() );

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const ImGuiID activeId{ GetActiveID() };
  const bool hovered{ window->IsHovered( clipRect ) };
  const bool isActive{ activeId == id };
  const bool mouseLeftJustPressed{ AppKeyboardApi::JustPressed( Key::MouseLeft ) };

  if( mouseLeftJustPressed && hovered && !isActive )
  {
    SetActiveID( id, window );
    textInputData->SetText( text );
  }

  if( mouseLeftJustPressed && !hovered && isActive )
    ClearActiveID();

  const v2 textPos{ pos + v2( buttonPadding, 0 ) };
  const v2 textBackgroundMaxi{ pos + v2( totalSize.x * ( 2.0f / 3.0f ), totalSize.y ) };

  const UI2DDrawData::Box box
  {
    .mMini  { pos },
    .mMaxi  { textBackgroundMaxi },
    .mColor { GetFrameColor( hovered ) },
  };

  drawData->AddBox( box, &clipRect );

  if( isActive )
  {
    TextInputDataUpdateKeys( textInputData, window->GetMousePosViewport(), textPos );

    // handle double click
    static GameTime lastMouseReleaseSeconds;
    static v2 lastMousePositionDesktopWindowspace;
    if( AppKeyboardApi::JustDepressed( Key::MouseLeft ) &&
        hovered &&
        !textInputData->mCodepoints.empty() )
    {
      const v2 screenspaceMousePos{ AppKeyboardApi::GetMousePosScreenspace() };
      const GameTime elapsedSecs{ ImGuiGlobals::Instance.mElapsedSeconds };
      const GameTimeDelta kDoubleClickSecs{ 0.5f };
      const bool releasedRecently{ elapsedSecs - lastMouseReleaseSeconds < kDoubleClickSecs };
      const bool releasedSamePos{ lastMousePositionDesktopWindowspace == screenspaceMousePos };
      if( releasedRecently )
      {
        if( releasedSamePos )
        {
          textInputData->mNumGlyphsBeforeCaret[ 0 ] = 0;
          textInputData->mNumGlyphsBeforeCaret[ 1 ] = textInputData->mCodepoints.size();
          textInputData->mCaretCount = 2;
        }
      }
      lastMouseReleaseSeconds = elapsedSecs;
      lastMousePositionDesktopWindowspace = screenspaceMousePos;
    }

    TextInputDataDrawSelection( textInputData, drawData, textPos, &clipRect );

    text = textInputData->GetText();
  }

  const UI2DDrawData::Text drawText
  {
    .mPos      { textPos },
    .mFontSize { ImGuiGetFontSize() },
    .mUtf8     { text },
    .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
  };
  drawData->AddText( drawText, &clipRect );

  const UI2DDrawData::Text labelText
  {
    .mPos      { v2( textBackgroundMaxi.x + itemSpacing.x, pos.y ) },
    .mFontSize { ImGuiGetFontSize() },
    .mUtf8     { label },
    .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
  };
  drawData->AddText( labelText, &clipRect );

  return oldText != text;
}

bool Tac::ImGuiSelectable( const StringView& str, bool selected )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };

  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };

  const float remainingWidth{ window->GetRemainingWidth() };

  const v2 buttonPosViewport{ window->mViewportSpaceCurrCursor };
  const v2 buttonSize( remainingWidth, ImGuiGetFontSize() );

  window->ItemSize( buttonSize );
  const ImGuiID id{ window->GetID( str ) };
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( buttonPosViewport, buttonSize ) };
  if( !window->Overlaps( origRect ) )
    return false;

  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( ShortFixedString::Concat( "ImGuiSelectable(", str, ")" ) );
  TAC_ON_DESTRUCT( drawData->PopDebugGroup() );

  const ImGuiRect clipRectViewport{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRectViewport ) };
  const bool active{ globals.mActiveID == id };
  const bool clicked{ hovered && AppKeyboardApi::JustPressed( Key::MouseLeft ) };
  if( clicked )
    SetActiveID( id, window );

  if( active && !AppKeyboardApi::IsPressed( Key::MouseLeft ) )
    ClearActiveID();

  if( selected || hovered )
  {
    drawData->AddBox(
      UI2DDrawData::Box
      {
        .mMini  { buttonPosViewport },
        .mMaxi  { buttonPosViewport + buttonSize },
        .mColor { GetFrameColor( hovered ) },
      }, &clipRectViewport );
  }
  drawData->AddText(
    UI2DDrawData::Text
    {
      .mPos      { buttonPosViewport },
      .mFontSize { ImGuiGetFontSize() },
      .mUtf8     { str },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &clipRectViewport );
  return clicked;
}

bool Tac::ImGuiInvisibleButton( const StringView& str, v2 size )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  ImGuiWindow* window{ globals.mCurrentWindow };

  dynmc v2 buttonSize{ size };
  if( size.x < 0 )
    buttonSize.x += window->GetRemainingWidth();
  if( size.y < 0 )
    buttonSize.y += window->GetRemainingHeight();

  const v2 pos{ window->mViewportSpaceCurrCursor };
  window->ItemSize( buttonSize );
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, buttonSize ) };
  if( !window->Overlaps( origRect ) )
    return false;

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRect ) };
  if( hovered )
  {
    const ImGuiID id{ window->GetID( str ) };
    SetHoveredID( id );
  }

  return hovered && AppKeyboardApi::JustPressed( Key::MouseLeft );
}

bool Tac::ImGuiButton( const StringView& str, v2 size )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  ImGuiWindow* window{ globals.mCurrentWindow };
  const float buttonPadding{ ImGuiGetButtonPadding() };
  const v2 textSize{ CalculateTextSize( str, ImGuiGetFontSize() ) };
  dynmc v2 buttonSize{ textSize + v2( 2 * buttonPadding, 0 ) };
  if( size.x != 0 )
    buttonSize.x = size.x;
  if( size.y != 0 )
    buttonSize.y = size.y;
  if( buttonSize.x < 0 )
    buttonSize.x += window->GetRemainingWidth();
  if( buttonSize.y < 0 )
    buttonSize.y += window->GetRemainingHeight();

  const v2 pos{ window->mViewportSpaceCurrCursor };
  window->ItemSize( buttonSize );

  // TODO: compare the various window->clip... apis against what dearimgui does
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, buttonSize ) };
  if( !window->Overlaps( origRect ) )
    return false;

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRect ) };
  if( hovered )
  {
    const ImGuiID id{ window->GetID( str ) };
    SetHoveredID( id );

    //static GameTime d;
    //Mouse::TryConsumeMouseMovement( &d, TAC_STACK_FRAME );
  }
  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( ShortFixedString::Concat( "Button(", str, ")" ) );
  drawData->AddBox(
    UI2DDrawData::Box
    {
      .mMini  { pos },
      .mMaxi  { pos + buttonSize },
      .mColor { GetFrameColor( hovered ) },
    }, &clipRect );
  drawData->AddText(
    UI2DDrawData::Text
    {
      .mPos      { pos + v2( buttonPadding, 0 ) },
      .mFontSize { ImGuiGetFontSize() },
      .mUtf8     { str },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &clipRect );
  drawData->PopDebugGroup();
  return hovered && AppKeyboardApi::JustPressed( Key::MouseLeft );
}

bool Tac::ImGuiCheckbox( const StringView& str, bool* value )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  ImGuiWindow* window{ globals.mCurrentWindow };
  const bool oldValue{ *value };
  const v2& itemSpacing{ ImGuiGetItemSpacing() };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  const v2 textSize{ CalculateTextSize( str, ImGuiGetFontSize() ) };
  const float boxWidth{ textSize.y };
  const v2 boxSize{ v2( 1, 1 ) * boxWidth };
  const v2 totalSize{ textSize + v2( boxWidth + itemSpacing.x, 0 ) };
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, totalSize ) };
  window->ItemSize( totalSize );
  if( !window->Overlaps( origRect ) )
    return false;

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRect ) };
  const Key lmb{ Key::MouseLeft };
  if( hovered && AppKeyboardApi::JustPressed( lmb ) )
  {
    *value = !*value;
    //Mouse::ButtonSetIsDown( lmb, false );
  }

  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( ShortFixedString::Concat( "Checkbox(", str ,")" ) );
  drawData->AddBox(
    UI2DDrawData::Box
    {
      .mMini  { pos },
      .mMaxi  { pos + boxSize },
      .mColor { GetFrameColor( hovered ) },
    }, &clipRect );

  if( *value )
    ImGuiDrawCheckMark( pos, boxWidth );

  drawData->AddText(
    UI2DDrawData::Text
    {
      .mPos      { pos + v2( boxWidth + itemSpacing.x, 0 ) },
      .mFontSize { ImGuiGetFontSize() },
      .mUtf8     { str },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &clipRect );
  drawData->PopDebugGroup();

  return oldValue != *value;
}

auto Tac::ImGuiGetCursorPos() -> Tac::v2
{
  return ImGuiGlobals::Instance.mCurrentWindow->mViewportSpaceCurrCursor;
}

void Tac::ImGuiSetCursorPos( const v2 local )
{
  ImGuiGlobals::Instance.mCurrentWindow->mViewportSpaceCurrCursor = local;
}

bool Tac::ImGuiIsRectHovered( ImGuiRect rect )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  return globals.mMouseHoveredWindow == globals.mCurrentWindow->GetWindowHandle()
    && globals.mCurrentWindow->IsHovered( rect );
}

auto Tac::ImGuiGetStyle() -> Tac::UIStyle&                { return ImGuiGlobals::Instance.mUIStyle; }
auto Tac::ImGuiGetColor( ImGuiCol col ) -> const Tac::v4& { return ImGuiGetStyle().colors[ ( int )col ]; }
void Tac::ImGuiSetColor( ImGuiCol colidx, v4 rgba )       { ImGuiGetStyle().colors[ ( int )colidx ] = rgba; }

void Tac::ImGuiDebugColors()
{
  ImGuiDefaultColors();
  ImGuiSetColor( ImGuiCol::Text, v4( 255, 255, 255, 255 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::WindowBackground, v4( 10, 20, 30, 255 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::ChildWindowBackground, v4( 5, 10, 15, 255 ) / 255.0f );
}

void Tac::ImGuiDefaultColors()
{
  const v4 unsetColor( -1.0f );

  UIStyle& style = ImGuiGetStyle();
  for( v4& color : style.colors )
    color = unsetColor;

  ImGuiSetColor( ImGuiCol::Text, v4( 218, 218, 218, 255 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::TextSelection, v4( 118, 178, 118, 155 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::WindowBackground, v4( 29, 30, 32, 255 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::ChildWindowBackground, v4( 15, 15, 15, 255 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::FrameBG, v4( 75, 104, 65, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::FrameBGHovered, v4( 115, 143, 91, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::FrameBGActive, v4( 128, 163, 85, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::Scrollbar, v4( 150, 150, 150, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::ScrollbarHovered, v4( 200, 200, 200, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::ScrollbarActive, v4( 60, 60, 60, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::ScrollbarBG, v4( 20, 20, 20, 255 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::Checkmark, v4( 249, 181, 53, 255 ) / 255.0f );

  for( v4& color : style.colors )
  {
    TAC_ASSERT( color != unsetColor );
  }
}

void Tac::ImGuiImage( const int hTex, const v2& size, const v4& color )
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  window->ItemSize( size );
  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( ShortFixedString::Concat( "ImGuiImage(", ToString( hTex ), ")" ) );
  drawData->AddBox(
    UI2DDrawData::Box
    {
      .mMini          { pos },
      .mMaxi          { pos + size },
      .mColor         { color },
      .mTextureHandle { Render::TextureHandle( hTex ) },
    } );
  drawData->PopDebugGroup();
}

auto Tac::ImGuiGetDrawData() -> Tac::UI2DDrawData*
{
  return ImGuiGlobals::Instance.mCurrentWindow->mDrawData;
}

bool Tac::ImGuiDragFloat( const StringView& s, float* v )  { return ImGuiDragFloatN( s, v, 1 ); }
bool Tac::ImGuiDragFloat2( const StringView& s, float* v ) { return ImGuiDragFloatN( s, v, 2 ); }
bool Tac::ImGuiDragFloat3( const StringView& s, float* v ) { return ImGuiDragFloatN( s, v, 3 ); }
bool Tac::ImGuiDragFloat4( const StringView& s, float* v ) { return ImGuiDragFloatN( s, v, 4 ); }
bool Tac::ImGuiDragInt( const StringView& s, int* v )      { return ImGuiDragIntN( s, v, 1 ); }
bool Tac::ImGuiDragInt2( const StringView& s, int* v )     { return ImGuiDragIntN( s, v, 2 ); }
bool Tac::ImGuiDragInt3( const StringView& s, int* v )     { return ImGuiDragIntN( s, v, 3 ); }
bool Tac::ImGuiDragInt4( const StringView& s, int* v )     { return ImGuiDragIntN( s, v, 4 ); }

bool Tac::ImGuiCollapsingHeader( const StringView& name, const ImGuiNodeFlags flags )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  ImGuiWindow* window{ globals.mCurrentWindow };
  const float buttonPadding{ ImGuiGetButtonPadding() };
  const float width{ window->GetRemainingWidth() };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  const v2 totalSize( width, ImGuiGetFontSize() );
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, totalSize ) };
  window->ItemSize( totalSize );
  if( !window->Overlaps( origRect ) )
    return false;

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRect ) };
  const ImGuiID id{ window->GetID( name ) };
  if( flags & ImGuiNodeFlags_DefaultOpen && !window->mCollapsingHeaderStates.contains( id ) )
    window->mCollapsingHeaderStates[ id ] = true;

  bool& isOpen{ window->mCollapsingHeaderStates[ id ] };
  if( hovered && AppKeyboardApi::JustPressed( Key::MouseLeft ) )
    isOpen = !isOpen;

  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( ShortFixedString::Concat( "ImGuiCollapsingHeader(", name, ")" ) );
  drawData->AddBox(
    UI2DDrawData::Box
    {
      .mMini  { pos },
      .mMaxi  { pos + totalSize },
      .mColor { GetFrameColor( hovered ) },
    }, &clipRect );
  ImGuiSetCursorPos( pos + v2( buttonPadding, 0 ) );
  ImGuiText( isOpen ? "v" : ">" );
  ImGuiSameLine();
  ImGuiText( name );
  drawData->PopDebugGroup();
  return isOpen;
}

void Tac::ImGuiPushFontSize( float value )
{
  ImGuiGlobals::Instance.mFontSizeSK.push_back( ImGuiGlobals::Instance.mUIStyle.fontSize );
  ImGuiGlobals::Instance.mUIStyle.fontSize = value;
}

void Tac::ImGuiPopFontSize()
{
  ImGuiGlobals::Instance.mUIStyle.fontSize = ImGuiGlobals::Instance.mFontSizeSK.back();
  ImGuiGlobals::Instance.mFontSizeSK.pop_back();
}

void Tac::ImGuiBeginMenuBar()
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  UI2DDrawData* drawData{ window->mDrawData };
  TAC_ASSERT( !window->mIsAppendingToMenu );
  window->mIsAppendingToMenu = true;
  drawData->AddBox(
    UI2DDrawData::Box
    {
      .mMini  {},
      .mMaxi  { v2( window->mSize.x, ImGuiGetFontSize() + ImGuiGetButtonPadding() * 2 ) },
      .mColor { v3( 69, 45, 83 ) / 255.0f, 1.0f },
    } );
}

void Tac::ImGuiEndMenuBar()
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  TAC_ASSERT( window->mIsAppendingToMenu );
  window->mIsAppendingToMenu = false;
}

void Tac::ImGuiDebugDraw()
{
  const ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  const ImGuiID activeId{ globals.mActiveID };
  const ImGuiID hoveredId{ globals.mHoveredID };
  const ImGuiWindow* activeIDWindow{ globals.mActiveIDWindow };
  const String activeIdWindowStr{ activeIDWindow ? activeIDWindow->mName : "null" };
  ImGuiText( String() + "hovered id: " + ToString( hoveredId.mValue ) );
  ImGuiText( String() + "active id: " + ToString( activeId.mValue ) );
  ImGuiText( String() + "active id window: " +  activeIdWindowStr );
}

void Tac::ImGuiBeginFrame( const BeginFrameData& data )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  globals.mElapsedSeconds = data.mElapsedSeconds;
  globals.mMouseHoveredWindow = data.mMouseHoveredWindow;
  globals.mMouseCursor = ImGuiMouseCursor::kArrow;
  globals.mHoveredID = {};

  if( ImGuiWindow * window{ globals.mMovingWindow } )
    window->UpdateMoveControls();

  if( globals.mSettingsDirty )
    for( ImGuiWindow* window : globals.mAllWindows )
        ImGuiSaveWindowSettings( window );
}

void Tac::ImGuiEndFrame( Errors& errors )
{
  TAC_UNUSED_PARAMETER( errors );
  TAC_PROFILE_BLOCK;

  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  for( const ImGuiWindow* window : globals.mWindowStack )
  {
    TAC_ASSERT_CRITICAL( String() + "Mismatched ImGuiBegin/ImGuiEnd for " + window->mName );
  }

  ImGuiDeleteWindows();

  for( ImGuiWindow* window : globals.mAllWindows )
    window->BeginMoveControls();
}

auto Tac::ImGuiGetFontSize() -> float             { return ImGuiGetStyle().fontSize; }
auto Tac::ImGuiGetItemSpacing() -> const Tac::v2& { return ImGuiGetStyle().itemSpacing; }
auto Tac::ImGuiGetButtonPadding() -> float        { return ImGuiGetStyle().buttonPadding; }

void Tac::ImGuiInit( const ImGuiInitParams& params, Errors& errors )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  globals.mMaxGpuFrameCount = params.mMaxGpuFrameCount;
  globals.mSettingsNode = params.mSettingsNode;
  TAC_ASSERT( globals.mMaxGpuFrameCount );
  TAC_ASSERT( globals.mSettingsNode.IsValid() );
  ImGuiPersistantPlatformData::Instance.Init( errors );
}

void Tac::ImGuiSaveWindowSettings( WindowHandle windowHandle )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  for( ImGuiWindow* window : globals.mAllWindows )
    if( window->mDesktopWindow->mWindowHandle == windowHandle )
      ImGuiSaveWindowSettings( window );
}

void Tac::ImGuiUninit()
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  for( ImGuiWindow* window : globals.mAllWindows )
    ImGuiSaveWindowSettings( window );
}

void Tac::ImGuiSetIsScrollbarEnabled( bool b ) { ImGuiGlobals::Instance.mScrollBarEnabled = b; }

auto Tac::ImGuiGetWindowPos() -> v2
{
    return ImGuiGlobals::Instance.mCurrentWindow->mViewportSpacePos;
}

auto Tac::ImGuiGetWindowContentRegionMin() -> v2
{
    return ImGuiGlobals::Instance.mCurrentWindow->mViewportSpaceVisibleRegion.mMini;
}

void Tac::ImGuiPlatformRender( Errors& errors )
{
  ImGuiPersistantPlatformData::Instance.UpdateAndRender( errors );
}

void Tac::ImGuiPlatformRenderFrameBegin( Errors& errors )
{
  Vector< ImGuiDesktopWindowImpl* >& desktopWindows{ ImGuiGlobals::Instance.mDesktopWindows };
  for( ImGuiDesktopWindowImpl* desktopWindow : desktopWindows ) //for( ImGuiSimWindowDraws& simDraw : simFrame->mWindowDraws )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const WindowHandle hDesktopWindow{ desktopWindow->mWindowHandle };
    if( !AppWindowApi::IsShown( hDesktopWindow ) )
      continue;

    TAC_CALL( Render::IContext::Scope renderContextScope{
      renderDevice->CreateRenderContext( errors ) } );
    Render::IContext* renderContext{ renderContextScope.GetContext() };

    const Render::SwapChainHandle fb{ AppWindowApi::GetSwapChainHandle( hDesktopWindow ) };

    const ShortFixedString renderGroupStr{ ShortFixedString::Concat(
      __FUNCTION__ , "(" , Tac::ToString( hDesktopWindow.GetIndex() ) , ")" ) };

    {
      TAC_RENDER_GROUP_BLOCK( renderContext, renderGroupStr );

      const Render::TextureHandle swapChainColor{ renderDevice->GetSwapChainCurrentColor( fb ) };
      const Render::TextureHandle swapChainDepth{ renderDevice->GetSwapChainDepth( fb ) };
      renderContext->ClearColor( swapChainColor, v4( 0, 0, 0, 1 ) );
      renderContext->ClearDepth( swapChainDepth, 1.0f );
    }
    TAC_CALL( renderContext->Execute(errors) );
  }
}

void Tac::ImGuiPlatformPresent( Errors& errors )
{
  Vector< ImGuiDesktopWindowImpl* >& desktopWindows{ ImGuiGlobals::Instance.mDesktopWindows };
  for( ImGuiDesktopWindowImpl* desktopWindow : desktopWindows ) 
  {
    const WindowHandle windowHandle{ desktopWindow->mWindowHandle };
    if( !AppWindowApi::IsShown( windowHandle ) )
      continue;

    const Render::SwapChainHandle swapChain { AppWindowApi::GetSwapChainHandle( windowHandle ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( renderDevice->Present( swapChain, errors ) );
  }
}

