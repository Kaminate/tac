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

#include <stdarg.h> // va_list, va_start, va_end
#include <stdio.h> // vsnprintf

namespace Tac
{

  struct ImGuiNextWindow
  {
    v2             mPosition          {};
    bool           mPositionValid     {};
    ImGuiCondition mPositionCondition {};
    v2             mSize              {};
    bool           mSizeValid         {};
    ImGuiCondition mSizeCondition     {};
    WindowHandle   mWindowHandle      {};
    bool           mStretch           {};
    bool           mMoveResize        {};
    bool           mEnableBG          { true }; // Set false to disable background render
    static ImGuiNextWindow gNextWindow;
  };

  ImGuiNextWindow             ImGuiNextWindow::gNextWindow;

  static auto ComputeLineCount( const StringView s ) -> int
  {
    // todo: word wrap
    int lineCount{ 1 };
    for( char c : s )
      if( c == '\n' )
        lineCount++;
    return lineCount;
  }

  static auto GetCaret( const CodepointString& codepoints, float mousePos ) -> int // mouse pos rel text top left corner
  {
    const int codepointCount{ codepoints.size() };

    float runningTextWidth{};
    int numGlyphsBeforeCaret{};
    for( int i{ 1 }; i <= codepointCount; ++i )
    {
      const v2 substringSize{ CalculateTextSize( codepoints.begin(), i, ImGuiGetFontSizePx() ) };
      const float lastGlyphWidth{ substringSize.x - runningTextWidth };
      const float lastGlyphMidpoint{ runningTextWidth + lastGlyphWidth / 2 };

      if( mousePos < lastGlyphMidpoint )
        break;

      runningTextWidth += lastGlyphWidth;
      numGlyphsBeforeCaret++;
    }
    return numGlyphsBeforeCaret;
  }

  static auto ImGuiGetWindowSettingsJson( const StringView name ) -> SettingsNode
  {
    SettingsNode windowsJson{ ImGuiGlobals::mSettingsNode.GetChild( "imgui.windows" ) };
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

  static void ImGuiSaveWindowWithSettings( const StringView name, int x, int y, int w, int h )
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
    
    ImGuiGlobals::mSettingsDirty = false;
    
    //if( !window->mWindowHandleOwned )
    //  return;

    WindowHandle h{ window->GetWindowHandle() };
    if( !AppWindowApi::IsShown( h ) )
      return;

    const v2i pos{ AppWindowApi::GetPos( h ) };
    const v2i size{ AppWindowApi::GetSize( h ) };
    ImGuiSaveWindowWithSettings( window->mName, pos.x, pos.y, size.x, size.y );
  }

  static auto GetFrameColor( const bool hovered ) -> const v4&
  {
    //
    const bool active{ hovered && UIKeyboardApi::IsPressed( Key::MouseLeft ) };
    const v4& boxColor{ ImGuiGetColor( active
                                       ? ImGuiCol::FrameBGActive : hovered
                                       ? ImGuiCol::FrameBGHovered : ImGuiCol::FrameBG ) };
    return boxColor;
  }

  static void ImGuiDeleteWindows()
  {
    dynmc int nAllWindows{ ImGuiGlobals::mAllWindows.size() };
    dynmc int iAllWindows{};
    while( iAllWindows < nAllWindows )
    {
      ImGuiWindow* window{ ImGuiGlobals::mAllWindows[ iAllWindows ] };
      const GameTime curSeconds{ ImGuiGlobals::mElapsedSeconds };
      const GameTimeDelta deletionWaitSeconds{ 0.1f };
      if( curSeconds > window->mRequestTime + deletionWaitSeconds )
      {
        // Remove from window array before destruction, because that goes through wndproc,
        // which goes back to imgui which iterates all windows
        ImGuiGlobals::mAllWindows[ iAllWindows ] = ImGuiGlobals::mAllWindows[ --nAllWindows ];
        ImGuiGlobals::mAllWindows.resize( nAllWindows );
        if( window->mWindowHandleOwned )
        {
          AppWindowApi::DestroyWindow( window->GetWindowHandle() );
        }
        TAC_DELETE window;
      }
      else
      {
        ++iAllWindows;
      }
    }
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
    ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
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

  //static Vector< UI2DDrawGpuInterface > sDrawInterfaces;
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
    if( UIKeyboardApi::JustPressed( keyMap.mKey ) )
      inputData->OnKeyPressed( keyMap.mTextInputKey );

  const CodepointView codepoints{ UIKeyboardApi::GetCodepoints() };
  for( Codepoint codepoint : codepoints )
    inputData->OnCodepoint( codepoint );

  if( UIKeyboardApi::JustPressed( Key::MouseLeft ) )
  {
    const int numGlyphsBeforeCaret = GetCaret( inputData->mCodepoints, mousePos.x - textPos.x );
    inputData->OnClick( numGlyphsBeforeCaret );
  }
  else if( UIKeyboardApi::IsPressed( Key::MouseLeft ) )
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
  const float fontSize{ ImGuiGetFontSizePx() };
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
    const double t{ ImGuiGlobals::mElapsedSeconds };
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
  return ImGuiGlobals::mMouseCursor;
}

auto Tac::ImGuiGetColName( const ImGuiCol colIdx ) -> const char*
{
  constexpr const char* names[]
  {
    "Text",
    "TextSelection",
    "WindowBackground",
    "ChildWindowBackground",
    "ResizeGrip",
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
  return ImGuiGlobals::mCurrentWindow->GetWindowHandle();
}

auto Tac::ImGuiGetWindowHandle( StringView name ) -> WindowHandle
{
  ImGuiWindow* window{ ImGuiGlobals::FindWindow( name ) };
  return window ? window->mDesktopWindow->mWindowHandle : WindowHandle{};
}

auto Tac::ImGuiGetContentRect() -> Tac::ImGuiRect
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
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
  return ImGuiGlobals::mCurrentWindow->GetID( label );
}

void Tac::PushID( StringView str )
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  const ImGuiID id{ window->GetID( str ) };
  window->mIDStack.push_back( id );
}

void Tac::PopID()
{
  ImGuiGlobals::mCurrentWindow->mIDStack.pop_back();
}


// In Dear ImGui, imgui.begin must be followed by imgui.end, regardless of imgui.begin return val
// In Tac ImGui, this is not the case.
bool Tac::ImGuiBegin( const StringView name, bool* open, ImGuiWindowFlags flags )
{
  ImGuiWindow* window{ ImGuiGlobals::FindWindow( name ) };
  if( open && !*open )
  {
    if( window )
    {
      int iWnd{};
      for( iWnd = 0; iWnd < ImGuiGlobals::mAllWindows.size(); ++iWnd )
        if( ImGuiGlobals::mAllWindows[ iWnd ] == window )
          break;
      TAC_ASSERT( iWnd != ImGuiGlobals::mAllWindows.size() );

      if( window->GetWindowHandle().IsValid() && window->mWindowHandleOwned )
      {
        int iImpl{};
        ImGuiDesktopWindowImpl* desktopWindowImpl{ ImGuiGlobals::FindDesktopWindow( window->GetWindowHandle() ) };
        TAC_ASSERT( desktopWindowImpl );
        for( iImpl = 0; iImpl < ImGuiGlobals::mDesktopWindows.size(); ++iImpl )
          if( ImGuiGlobals::mDesktopWindows[ iImpl ] == desktopWindowImpl )
            break;
        TAC_ASSERT( iWnd != ImGuiGlobals::mDesktopWindows.size() );

        Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
        for( Render::BufferHandle b : desktopWindowImpl->mRenderBuffers.mVBs )
          renderDevice->DestroyBuffer( b );
        for( Render::BufferHandle b : desktopWindowImpl->mRenderBuffers.mIBs )
          renderDevice->DestroyBuffer( b );
        desktopWindowImpl->mRenderBuffers = {};

        AppWindowApi::DestroyWindow( window->GetWindowHandle() );

        TAC_DELETE desktopWindowImpl;
        Swap( ImGuiGlobals::mDesktopWindows[ iImpl ], ImGuiGlobals::mDesktopWindows.back() );
        ImGuiGlobals::mDesktopWindows.pop_back();
      }

      TAC_DELETE window;
      Swap( ImGuiGlobals::mAllWindows[ iWnd ], ImGuiGlobals::mAllWindows.back() );
      ImGuiGlobals::mAllWindows.pop_back();
    }

    ImGuiNextWindow::gNextWindow = {};
    return false;
  }

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


      SettingsNode windowJson{ ImGuiGetWindowSettingsJson( name ) };
      x = ( int )windowJson.GetChild( "x" ).GetValueWithFallback( ( JsonNumber )x ).mNumber;
      y = ( int )windowJson.GetChild( "y" ).GetValueWithFallback( ( JsonNumber )y ).mNumber;
      w = ( int )windowJson.GetChild( "w" ).GetValueWithFallback( ( JsonNumber )w ).mNumber;
      h = ( int )windowJson.GetChild( "h" ).GetValueWithFallback( ( JsonNumber )h ).mNumber;

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
      = ImGuiGlobals::FindDesktopWindow( hDesktopWindow );
    if( !imguiDesktopWindow )
    {
      imguiDesktopWindow = TAC_NEW ImGuiDesktopWindowImpl;
      imguiDesktopWindow->mWindowHandle = hDesktopWindow;
      ImGuiGlobals::mDesktopWindows.push_back( imguiDesktopWindow );
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
    window->mPreviousViewportToggleSize = size;
    window->mStretchWindow = ImGuiNextWindow::gNextWindow.mStretch;
    window->mMoveResizeWindow = ImGuiNextWindow::gNextWindow.mMoveResize;
    window->mEnableBG = ImGuiNextWindow::gNextWindow.mEnableBG;
    ImGuiGlobals::mAllWindows.push_back( window );
  }


  ImGuiNextWindow::gNextWindow = {};

  TAC_ASSERT( window->mSize.x > 0 && window->mSize.y > 0 );

  ImGuiDesktopWindow* desktopWindow{ window->mDesktopWindow };
  const WindowHandle hWindow { desktopWindow->mWindowHandle };
  window->mRequestTime = ImGuiGlobals::mElapsedSeconds;
  if( !AppWindowApi::IsShown( hWindow ) )
    return false;

  ImGuiGlobals::mWindowStack.push_back( window );
  ImGuiGlobals::mCurrentWindow = window;

  if( window->mStretchWindow )
    window->mSize = AppWindowApi::GetSize( hWindow );

  window->mFlags = flags;
  window->mOpen = open;
  window->mPopupMenu = "";
  window->BeginFrame();

  return true;
}

void Tac::ImGuiEnd()
{
  ImGuiGlobals::mWindowStack.back()->EndFrame();
  ImGuiGlobals::mWindowStack.pop_back();
  ImGuiGlobals::mCurrentWindow
    = ImGuiGlobals::mWindowStack.empty()
    ? nullptr
    : ImGuiGlobals::mWindowStack.back();
}

void Tac::ImGuiBeginChild( StringView name, const v2& size )
{
  ImGuiWindow* parent{ ImGuiGlobals::mCurrentWindow };
  TAC_ASSERT( parent );
  ImGuiWindow* child{ ImGuiGlobals::FindWindow( name ) };
  if( !child )
  {
    child = TAC_NEW ImGuiWindow;
    child->mName = name;
    child->mParent = parent;
    child->mDrawData = parent->mDrawData;
    child->mTextInputData = parent->mTextInputData;
    child->mDesktopWindow = parent->mDesktopWindow;
    ImGuiGlobals::mAllWindows.push_back( child );
  }
  child->mRequestTime = ImGuiGlobals::mElapsedSeconds;
  child->mSize = v2( size.x > 0 ? size.x : size.x + parent->mSize.x,
                     size.y > 0 ? size.y : size.y + parent->mSize.y );
  ImGuiGlobals::mWindowStack.push_back( child );
  ImGuiGlobals::mCurrentWindow = child;
  child->BeginFrame();
}

void Tac::ImGuiEndChild()
{
  
  ImGuiWindow* child{ ImGuiGlobals::mCurrentWindow };
  child->mParent->ItemSize( child->mSize );
  child->EndFrame();
  ImGuiGlobals::mWindowStack.pop_back();
  ImGuiGlobals::mCurrentWindow = ImGuiGlobals::mWindowStack.back();
}

void Tac::ImGuiBeginGroup()
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  window->PushXOffset();
  window->mCurrLineHeight = 0;
  window->mGroupStack.push_back(
    GroupData
    {
      .mSavedCursorDrawPos { window->mViewportSpaceCurrCursor },
      .mSavedLineHeight    { window->mCurrLineHeight },
    } );
}

void Tac::ImGuiEndGroup()
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  const GroupData& groupData{ window->mGroupStack.back() };
  const v2 groupSize{ window->mViewportSpaceMaxiCursor - groupData.mSavedCursorDrawPos };

  window->mXOffsets.pop_back();
  window->mCurrLineHeight = groupData.mSavedLineHeight;
  window->mViewportSpaceCurrCursor = groupData.mSavedCursorDrawPos;
  window->ItemSize( groupSize );
  window->mGroupStack.pop_back();
}

void Tac::ImGuiIndent()
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  window->mViewportSpaceCurrCursor.x += 15.0f;
  window->PushXOffset();
}

void Tac::ImGuiUnindent()
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  window->mXOffsets.pop_back();
  window->mViewportSpaceCurrCursor.x = window->mViewportSpacePos.x + window->mXOffsets.back();
}

void Tac::ImGuiSameLine()
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  window->mViewportSpaceCurrCursor
    = window->mViewportSpacePrevCursor
    + v2( ImGuiGetItemSpacingPx().x, 0 );
  window->mCurrLineHeight = window->mPrevLineHeight;
}

void Tac::ImGuiSeparator()
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  const float w{ window->GetRemainingWidth() };
  const float h{ 1 }; // Tac::ImGuiGetFontSizePx() / 8
  const v2 rectSize(w,h);
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( window->mViewportSpaceCurrCursor, rectSize ) };
  window->ItemSize( rectSize );
  if( !window->Overlaps( origRect ) )
    return;
  const ImGuiRect clipRect{ window->Clip( origRect ) };
  window->mDrawData->AddBox(
    UI2DDrawData::Box
    {
      .mMini     { clipRect.mMini },
      .mMaxi     { clipRect.mMaxi },
      .mColor    { 1, 1, 0, .2f },
    }, &clipRect );
}

void Tac::ImGuiText( const char* fmt, ... )//const StringView utf8 )
{
  const size_t bufSize{ 1024 };
  char buf[ 1024 ];
  va_list args;
  va_start( args, fmt );
  int n{ vsnprintf( buf, bufSize, fmt, args ) };
  ImGuiText( n >= 0 ? StringView( buf, n ) : StringView( "<encoding error>" ) );
  va_end( args );
}

void Tac::ImGuiText( StringView utf8 )
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  UI2DDrawData* drawData{ window->mDrawData };
  const v2 textPos{ window->mViewportSpaceCurrCursor };
  const float fontSizePx{ Tac::ImGuiGetFontSizePx() };
  const v2 textSize{ CalculateTextSize( utf8, fontSizePx ) };
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
      .mFontSize { fontSizePx },
      .mUtf8     { utf8 },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &clipRect );
  drawData->PopDebugGroup();
}

bool Tac::ImGuiInputText( const StringView label, String& text )
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };

  const float buttonPaddingPx{ ImGuiGetButtonPaddingPx() };
  const v2 itemSpacing{ ImGuiGetItemSpacingPx() };
  const GameTime mouseReleaseSeconds{ ImGuiGlobals::mElapsedSeconds };
  const ImGuiID id{ window->GetID( label ) };
  const String oldText{ text };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  const int lineCount{ ComputeLineCount( text ) };
  const float width { window->GetRemainingWidth() };
  const v2 totalSize( width, lineCount * ImGuiGetFontSizePx() );
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, totalSize ) };

  window->ItemSize( totalSize );
  if( !window->Overlaps( origRect ) )
    return false;

  TextInputData* textInputData{ window->mTextInputData };
  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( ShortFixedString::Concat( "ImGuiInputText(" , label , ")" ) );
  TAC_ON_DESTRUCT( drawData->PopDebugGroup() );

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const ImGuiID activeId{ ImGuiGlobals::GetActiveID() };
  const bool hovered{ window->IsHovered( clipRect, id ) };
  const bool isActive{ activeId == id };
  const bool mouseLeftJustPressed{ UIKeyboardApi::JustPressed( Key::MouseLeft ) };

  if( mouseLeftJustPressed && hovered && !isActive )
  {
    ImGuiGlobals::SetActiveID( id, window );
    textInputData->SetText( text );
  }

  if( mouseLeftJustPressed && !hovered && isActive )
    ImGuiGlobals::ClearActiveID();

  const v2 textPos{ pos + v2( buttonPaddingPx, 0 ) };
  const v2 textBackgroundMaxi{ pos + v2( totalSize.x * ( 2.0f / 3.0f ), totalSize.y ) };
  drawData->AddBox(
    UI2DDrawData::Box
    {
      .mMini  { pos },
      .mMaxi  { textBackgroundMaxi },
      .mColor { GetFrameColor( hovered ) },
    }, &clipRect );

  if( isActive )
  {
    TextInputDataUpdateKeys( textInputData, window->GetMousePos_uiwindowspace(), textPos );

    // handle double click
    static GameTime lastMouseReleaseSeconds;
    static v2 lastMousePositionDesktopWindowspace;
    if( UIKeyboardApi::JustDepressed( Key::MouseLeft ) &&
        hovered &&
        !textInputData->mCodepoints.empty() )
    {
      const v2 screenspaceMousePos{ UIKeyboardApi::GetMousePosScreenspace() };
      const GameTime elapsedSecs{ ImGuiGlobals::mElapsedSeconds };
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
    .mFontSize { ImGuiGetFontSizePx() },
    .mUtf8     { text },
    .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
  };
  drawData->AddText( drawText, &clipRect );

  const UI2DDrawData::Text labelText
  {
    .mPos      { v2( textBackgroundMaxi.x + itemSpacing.x, pos.y ) },
    .mFontSize { ImGuiGetFontSizePx() },
    .mUtf8     { label },
    .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
  };
  drawData->AddText( labelText, &clipRect );

  return oldText != text;
}

bool Tac::ImGuiSelectable( const StringView str, bool selected )
{
  

  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };

  const float remainingWidth{ window->GetRemainingWidth() };

  const v2 buttonPosViewport{ window->mViewportSpaceCurrCursor };
  const v2 buttonSize( remainingWidth, ImGuiGetFontSizePx() );

  window->ItemSize( buttonSize );
  const ImGuiID id{ window->GetID( str ) };
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( buttonPosViewport, buttonSize ) };
  if( !window->Overlaps( origRect ) )
    return false;

  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( ShortFixedString::Concat( "ImGuiSelectable(", str, ")" ) );
  TAC_ON_DESTRUCT( drawData->PopDebugGroup() );

  const ImGuiRect clipRectViewport{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRectViewport, id ) };
  const bool active{ ImGuiGlobals::mActiveID == id };
  const bool clicked{ hovered && UIKeyboardApi::JustPressed( Key::MouseLeft ) };
  if( clicked )
    ImGuiGlobals::SetActiveID( id, window );

  if( active && !UIKeyboardApi::IsPressed( Key::MouseLeft ) )
    ImGuiGlobals::ClearActiveID();

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
      .mFontSize { ImGuiGetFontSizePx() },
      .mUtf8     { str },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &clipRectViewport );
  return clicked;
}

bool Tac::ImGuiInvisibleButton( const StringView str, v2 size )
{
  
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };

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

    const ImGuiID id{ window->GetID( str ) };
  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRect, id ) };

  return hovered && UIKeyboardApi::JustPressed( Key::MouseLeft );
}

bool Tac::ImGuiButton( const StringView str, v2 size )
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  const float buttonPaddingPx{ ImGuiGetButtonPaddingPx() };
  const v2 textSize{ CalculateTextSize( str, ImGuiGetFontSizePx() ) };
  dynmc v2 buttonSize{ textSize + v2( 2 * buttonPaddingPx, 0 ) };
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

  const ImGuiID id{ window->GetID( str ) };
  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRect, id ) };
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
      .mPos      { pos + v2( buttonPaddingPx, 0 ) },
      .mFontSize { ImGuiGetFontSizePx() },
      .mUtf8     { str },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &clipRect );
  drawData->PopDebugGroup();
  return hovered && UIKeyboardApi::JustPressed( Key::MouseLeft );
}

bool Tac::ImGuiCheckbox( const StringView str, bool* value )
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  const ImGuiID id{ window->GetID( str ) };
  const bool oldValue{ *value };
  const v2 itemSpacing{ ImGuiGetItemSpacingPx() };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  const v2 textSize{ CalculateTextSize( str, ImGuiGetFontSizePx() ) };
  const float boxWidth{ textSize.y };
  const v2 boxSize{ v2( 1, 1 ) * boxWidth };
  const v2 totalSize{ textSize + v2( boxWidth + itemSpacing.x, 0 ) };
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, totalSize ) };
  window->ItemSize( totalSize );
  if( !window->Overlaps( origRect ) )
    return false;

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRect, id ) };
  const Key lmb{ Key::MouseLeft };
  if( hovered && UIKeyboardApi::JustPressed( lmb ) )
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
      .mFontSize { ImGuiGetFontSizePx() },
      .mUtf8     { str },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &clipRect );
  drawData->PopDebugGroup();

  return oldValue != *value;
}

auto Tac::ImGuiGetCursorPos() -> Tac::v2
{
  return ImGuiGlobals::mCurrentWindow->mViewportSpaceCurrCursor;
}

void Tac::ImGuiSetCursorPos( const v2 local )
{
  ImGuiGlobals::mCurrentWindow->mViewportSpaceCurrCursor = local;
}


auto Tac::ImGuiGetStyle() -> Tac::UIStyle&                { return ImGuiGlobals::mUIStyle; }
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

  ImGuiSetColor( ImGuiCol::Text,                  v4( 218, 218, 218, 255 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::TextSelection,         v4( 118, 178, 118, 155 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::WindowBackground,      v4( 29, 30, 32, 255 )    / 255.0f );
  ImGuiSetColor( ImGuiCol::ChildWindowBackground, v4( 15, 15, 15, 255 )    / 255.0f );
  ImGuiSetColor( ImGuiCol::ResizeGrip,            v4( 70, 54, 25, 255 )    / 255.0f );
  ImGuiSetColor( ImGuiCol::FrameBG,               v4( 75, 104, 65, 128 )   / 255.0f );
  ImGuiSetColor( ImGuiCol::FrameBGHovered,        v4( 115, 143, 91, 128 )  / 255.0f );
  ImGuiSetColor( ImGuiCol::FrameBGActive,         v4( 128, 163, 85, 128 )  / 255.0f );
  ImGuiSetColor( ImGuiCol::Scrollbar,             v4( 150, 150, 150, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::ScrollbarHovered,      v4( 200, 200, 200, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::ScrollbarActive,       v4( 60, 60, 60, 128 )    / 255.0f );
  ImGuiSetColor( ImGuiCol::ScrollbarBG,           v4( 20, 20, 20, 255 )    / 255.0f );
  ImGuiSetColor( ImGuiCol::Checkmark,             v4( 249, 181, 53, 255 )  / 255.0f );

  for( v4& color : style.colors )
  {
    TAC_ASSERT( color != unsetColor );
  }
}

void Tac::ImGuiImage( const int hTex, const v2& size, const v4& color )
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
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
  return ImGuiGlobals::mCurrentWindow->mDrawData;
}

bool Tac::ImGuiDragFloat( const StringView s, float* v )  { return ImGuiDragFloatN( s, v, 1 ); }
bool Tac::ImGuiDragFloat2( const StringView s, float* v ) { return ImGuiDragFloatN( s, v, 2 ); }
bool Tac::ImGuiDragFloat3( const StringView s, float* v ) { return ImGuiDragFloatN( s, v, 3 ); }
bool Tac::ImGuiDragFloat4( const StringView s, float* v ) { return ImGuiDragFloatN( s, v, 4 ); }
bool Tac::ImGuiDragInt( const StringView s, int* v )      { return ImGuiDragIntN( s, v, 1 ); }
bool Tac::ImGuiDragInt2( const StringView s, int* v )     { return ImGuiDragIntN( s, v, 2 ); }
bool Tac::ImGuiDragInt3( const StringView s, int* v )     { return ImGuiDragIntN( s, v, 3 ); }
bool Tac::ImGuiDragInt4( const StringView s, int* v )     { return ImGuiDragIntN( s, v, 4 ); }

bool Tac::ImGuiCollapsingHeader( const StringView name, const ImGuiNodeFlags flags )
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  const float buttonPaddingPx{ ImGuiGetButtonPaddingPx() };
  const float width{ window->GetRemainingWidth() };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  const v2 totalSize( width, ImGuiGetFontSizePx() );
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, totalSize ) };
  window->ItemSize( totalSize );
  if( !window->Overlaps( origRect ) )
    return false;

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const ImGuiID id{ window->GetID( name ) };
  const bool hovered{ window->IsHovered( clipRect, id ) };
  if( flags & ImGuiNodeFlags_DefaultOpen && !window->mCollapsingHeaderStates.contains( id ) )
    window->mCollapsingHeaderStates[ id ] = true;

  bool& isOpen{ window->mCollapsingHeaderStates[ id ] };
  if( hovered && UIKeyboardApi::JustPressed( Key::MouseLeft ) )
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
  ImGuiSetCursorPos( pos + v2( buttonPaddingPx, 0 ) );
  ImGuiText( isOpen ? "v" : ">" );
  ImGuiSameLine();
  ImGuiText( name );
  drawData->PopDebugGroup();
  return isOpen;
}

void Tac::ImGuiPushFontSize( float value )
{
  ImGuiGlobals::mFontSizeStack.push_back( ImGuiGlobals::mUIStyle.fontSize );
  ImGuiGlobals::mUIStyle.fontSize = value;
}

void Tac::ImGuiPopFontSize()
{
  ImGuiGlobals::mUIStyle.fontSize = ImGuiGlobals::mFontSizeStack.back();
  ImGuiGlobals::mFontSizeStack.pop_back();
}

bool Tac::ImGuiBeginMenu( const StringView str )
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };

  const v2 textSize{ CalculateTextSize( str, ImGuiGetFontSizePx() ) };
  const float paddingPx{ 5 * ImGuiGetButtonPaddingPx() };
  dynmc v2 buttonSize{ textSize + v2( 2 * paddingPx, 0 ) };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  window->ItemSize( buttonSize );

  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, buttonSize ) };
  if( !window->Overlaps( origRect ) )
    return false;

  const ImGuiID id{ window->GetID( str ) };
  const bool hovered{ window->IsHovered( origRect, id ) };
  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( ShortFixedString::Concat( "Button(", str, ")" ) );
  drawData->AddBox(
    UI2DDrawData::Box
    {
      .mMini  { pos },
      .mMaxi  { pos + buttonSize },
      .mColor { GetFrameColor( hovered ) },
    }, &origRect );

  drawData->AddText(
    UI2DDrawData::Text
    {
      .mPos      { pos + v2( paddingPx, 0 ) },
      .mFontSize { ImGuiGetFontSizePx() },
      .mUtf8     { str },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &origRect );
  drawData->PopDebugGroup();
  ImGuiSameLine();


  window->mMenuBarCurrWidth += buttonSize.x;
  window->mViewportSpaceCurrCursor.x -= ImGuiGetItemSpacingPx().x;


  String popupName{ str + "_popup"};
  int popupIdx{ -1 };
  for( int i{}; i < ImGuiGlobals::mPopupStack.size(); ++i )
    if( ImGuiGlobals::mPopupStack[ i ] == popupName )
      popupIdx = i;

  if( hovered && UIKeyboardApi::JustPressed( Key::MouseLeft ) )
  {
    if( popupIdx == -1 )
    {
      ImGuiGlobals::mPopupStack.clear();
      ImGuiGlobals::mPopupStack.push_back( popupName );
      popupIdx = 0;
    }
    else
    {
      ImGuiGlobals::mPopupStack.resize( popupIdx );
      popupIdx = -1;
    }
  }

  if( popupIdx != -1 )
  {
    window->mPopupMenu = popupName;
    ImGuiWindowFlags flags{ ImGuiWindowFlags( 0
      | ImGuiWindowFlags_NoTitleBar
      | ImGuiWindowFlags_NoResize
      | ImGuiWindowFlags_AutoResize
    ) };

    ImGuiSetNextWindowPosition(
      AppWindowApi::GetPos(window->mDesktopWindow->mWindowHandle) +  pos + v2( 0, textSize.y ) );
    return ImGuiBegin( popupName, nullptr, flags );
  }

  return false;
}

void Tac::ImGuiEndMenu()
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };

  int popupIdx{ -1 };
  for( int i{}; i < ImGuiGlobals::mPopupStack.size(); ++i )
    if( ImGuiGlobals::mPopupStack[ i ] == window->mName )
      popupIdx = i;

  if( popupIdx != -1 )
    ImGuiEnd();
}

bool Tac::ImGuiBeginMenuBar()
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  if( window->mFlags & ImGuiWindowFlags_NoTitleBar )
    return false;
  TAC_ASSERT(!window->mAppendingToMenuBar);
  window->mAppendingToMenuBar = true;
  window->mMenuBarCachedCursor = window->mViewportSpaceCurrCursor;
  window->mViewportSpaceCurrCursor = window->mMenuBarMini;
  return true;
}

void Tac::ImGuiEndMenuBar()
{
  ImGuiWindow* window{ ImGuiGlobals::mCurrentWindow };
  TAC_ASSERT(window->mAppendingToMenuBar);
  TAC_ASSERT( !( window->mFlags & ImGuiWindowFlags_NoTitleBar ) );
  window->mAppendingToMenuBar = false;
  window->mViewportSpaceCurrCursor = window->mMenuBarCachedCursor;
  window->mMenuBarPrevWidth = window->mMenuBarCurrWidth;
  window->mMenuBarCurrWidth = 0;
}


void Tac::ImGuiDebugDraw()
{
  const ImGuiID activeId{ ImGuiGlobals::mActiveID };
  const ImGuiID hoveredId{ ImGuiGlobals::mHoveredID };
  const ImGuiWindow* activeIDWindow{ ImGuiGlobals::mActiveIDWindow };
  const String activeIdWindowStr{ activeIDWindow ? activeIDWindow->mName : "null" };
  ImGuiText( String() + "hovered id: " + ToString( hoveredId.mValue ) );
  ImGuiText( String() + "active id: " + ToString( activeId.mValue ) );
  ImGuiText( String() + "active id window: " +  activeIdWindowStr );
}

void Tac::ImGuiBeginFrame( const BeginFrameData& data )
{
  ImGuiGlobals::mElapsedSeconds = data.mElapsedSeconds;
  ImGuiGlobals::mMouseHoveredWindow = data.mMouseHoveredWindow;
  ImGuiGlobals::mMouseCursor = ImGuiMouseCursor::kArrow;
  ImGuiGlobals::mHoveredID = {};

  for( ImGuiDesktopWindowImpl* desktopWindow : ImGuiGlobals::mDesktopWindows )
  {
    if( desktopWindow->mMonitorDpiRequested )
    {
      const void* nwh{ AppWindowApi::GetNativeWindowHandle( desktopWindow->mWindowHandle ) };
      const Monitor monitor{ OS::OSGetMonitorFromNativeWindowHandle( nwh ) };
      desktopWindow->mMonitorDpi = ( float )monitor.mDpi;
      desktopWindow->mMonitorDpiRequested = false;
    }
  }

  if( ImGuiWindow * window{ ImGuiGlobals::mMovingWindow } )
    window->UpdateMoveControls();

  if( ImGuiGlobals::mSettingsDirty )
    for( ImGuiWindow* window : ImGuiGlobals::mAllWindows )
        ImGuiSaveWindowSettings( window );
}

void Tac::ImGuiEndFrame( Errors& errors )
{
  TAC_UNUSED_PARAMETER( errors );
  TAC_PROFILE_BLOCK;

  
  for( const ImGuiWindow* window : ImGuiGlobals::mWindowStack )
  {
    TAC_ASSERT_CRITICAL( String() + "Mismatched ImGuiBegin/ImGuiEnd for " + window->mName );
  }

  ImGuiDeleteWindows();

  for( ImGuiWindow* window : ImGuiGlobals::mAllWindows )
    window->BeginMoveControls();

  if( ImGuiGlobals::mHoveredIDPrev && !ImGuiGlobals::mHoveredID )
    ImGuiGlobals::mHoverStartTime = {};
  ImGuiGlobals::mHoveredIDPrev = ImGuiGlobals::mHoveredID;

  // true during text input, false otherwise
  UIKeyboardApi::sWantCaptureKeyboard = ImGuiGlobals::mHoveredIDPrev.IsValid();
  //UIKeyboardApi::sWantCaptureMouse = ImGuiGlobals::mHoveredIDPrev.IsValid();
}
auto Tac::ImGuiGetFontSizePx() -> float           
{
  return ImGuiGetStyle().fontSize
    * ImGuiGlobals::mCurrentWindow->mDesktopWindow->mMonitorDpi
    / ImGuiGlobals::mReferenceResolution.mDpi;
}

auto Tac::ImGuiGetWindowPaddingPx() -> float
{
  return ImGuiGetStyle().windowPadding
    * ImGuiGlobals::mCurrentWindow->mDesktopWindow->mMonitorDpi
    / ImGuiGlobals::mReferenceResolution.mDpi;
}
auto Tac::ImGuiGetItemSpacingPx() -> Tac::v2
{
  return ImGuiGetStyle().itemSpacing
    * ImGuiGlobals::mCurrentWindow->mDesktopWindow->mMonitorDpi
    / ImGuiGlobals::mReferenceResolution.mDpi;
}
auto Tac::ImGuiGetButtonPaddingPx() -> float
{
  return ImGuiGetStyle().buttonPadding
    * ImGuiGlobals::mCurrentWindow->mDesktopWindow->mMonitorDpi
    / ImGuiGlobals::mReferenceResolution.mDpi;
}

void Tac::ImGuiInit( const ImGuiInitParams& params, Errors& )
{
  
  ImGuiGlobals::mMaxGpuFrameCount = params.mMaxGpuFrameCount;
  ImGuiGlobals::mSettingsNode = params.mSettingsNode;
  TAC_ASSERT( ImGuiGlobals::mMaxGpuFrameCount );
  TAC_ASSERT( ImGuiGlobals::mSettingsNode.IsValid() );
}

void Tac::ImGuiSaveWindowSettings( WindowHandle windowHandle )
{
  for( ImGuiWindow* window : ImGuiGlobals::mAllWindows )
    if( window->mDesktopWindow->mWindowHandle == windowHandle )
      ImGuiSaveWindowSettings( window );
}

void Tac::ImGuiUninit()
{
  
  for( ImGuiWindow* window : ImGuiGlobals::mAllWindows )
    ImGuiSaveWindowSettings( window );
}

void Tac::ImGuiSetIsScrollbarEnabled( bool b ) { ImGuiGlobals::mScrollBarEnabled = b; }

auto Tac::ImGuiGetWindowPos() -> v2
{
    return ImGuiGlobals::mCurrentWindow->mViewportSpacePos;
}

auto Tac::ImGuiGetWindowContentRegionMin() -> v2
{
    return ImGuiGlobals::mCurrentWindow->mViewportSpaceVisibleRegion.mMini;
}


void Tac::ImGuiPlatformRenderFrameBegin( Errors& errors )
{
  Vector< ImGuiDesktopWindowImpl* >& desktopWindows{ ImGuiGlobals::mDesktopWindows };
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

void Tac::ImGuiPlatformHandleDpiChange( WindowHandle h )
{
  for( ImGuiDesktopWindowImpl* desktopWindow : ImGuiGlobals::mDesktopWindows )
    if( desktopWindow->mWindowHandle == h )
      desktopWindow->mMonitorDpiRequested = true;
}

void Tac::ImGuiPlatformPresent( Errors& errors )
{
  for( ImGuiDesktopWindowImpl* desktopWindow : ImGuiGlobals::mDesktopWindows ) 
  {
    const WindowHandle windowHandle{ desktopWindow->mWindowHandle };
    if( !AppWindowApi::IsShown( windowHandle ) )
      continue;

    const Render::SwapChainHandle swapChain { AppWindowApi::GetSwapChainHandle( windowHandle ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( renderDevice->Present( swapChain, errors ) );
  }
}

