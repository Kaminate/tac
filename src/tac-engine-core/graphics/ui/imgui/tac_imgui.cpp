#include "tac_imgui.h" // self-inc

#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_drag.h"
#include "tac-engine-core/graphics/ui/tac_text_edit.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
//#include "tac-engine-core/hid/tac_keyboard_api.h"
#include "tac-engine-core/profile/tac_profile.h"
//#include "tac-engine-core/settings/tac_settings.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"

//#include "tac-rhi/render3/tac_render_api.h"
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

//#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
//#include "tac-engine-core/window/tac_window_api_graphics.h"

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
    const float fontSize{ ImGuiGetFontSize() };
    const int codepointCount{ codepoints.size() };

    float runningTextWidth{};
    int numGlyphsBeforeCaret{};
    for( int i{ 1 }; i <= codepointCount; ++i )
    {
      const v2 substringSize{ CalculateTextSize( codepoints.begin(), i, fontSize ) };
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
    SimWindowApi windowApi{ globals.mSimWindowApi };
    //if( !window->mWindowHandleOwned )
    //  return;

    WindowHandle h{ window->GetWindowHandle() };
    if( !windowApi.IsShown( h ) )
      return;

    const v2i pos{ windowApi.GetPos( h ) };
    const v2i size{ windowApi.GetSize( h ) };
    ImGuiSaveWindowWithSettings( window->mName, pos.x, pos.y, size.x, size.y );
  }

  static const v4& GetFrameColor( const bool hovered )
  {
    ImGuiGlobals& globals{ ImGuiGlobals::Instance };
    SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };

    const bool active{ hovered && keyboardApi.IsPressed( Key::MouseLeft ) };
    const v4& boxColor{ ImGuiGetColor( active
                                       ? ImGuiCol::FrameBGActive : hovered
                                       ? ImGuiCol::FrameBGHovered : ImGuiCol::FrameBG ) };
    return boxColor;
  }


  // -----------------------------------------------------------------------------------------------

  UIStyle::UIStyle()
  {
    ImGuiDefaultColors();
  }

  // -----------------------------------------------------------------------------------------------

  ImGuiRect ImGuiRect::FromPosSize( v2 pos, v2 size )
  {
    return
    {
      .mMini { pos },
      .mMaxi { pos + size },
    };
  }

  ImGuiRect ImGuiRect::FromMinMax( v2 mini, v2 maxi )
  {
    return
    {
      .mMini { mini },
      .mMaxi { maxi },
    };
  }

  float     ImGuiRect::GetWidth() const
  {
    return mMaxi.x - mMini.x;
  }

  float     ImGuiRect::GetHeight() const
  {
    return mMaxi.y - mMini.y;
  }

  v2        ImGuiRect::GetSize() const
  {
    return mMaxi - mMini;
  }

  bool      ImGuiRect::ContainsPoint( v2 p ) const
  {
    return
      p.x > mMini.x &&
      p.x < mMaxi.x &&
      p.y > mMini.y &&
      p.y < mMaxi.y;
  }

  bool      ImGuiRect::Contains( const ImGuiRect& r ) const
  {
    return
      r.mMini.x >= mMini.x &&
      r.mMaxi.x <= mMaxi.x &&
      r.mMini.y >= mMini.y &&
      r.mMaxi.y <= mMaxi.y;
  }

  bool      ImGuiRect::Overlaps( const ImGuiRect& r ) const
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

void Tac::TextInputDataUpdateKeys( TextInputData* inputData,
                                   const v2& mousePos,
                                   const v2& textPos )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };

  struct KeyMap
  {
    Key mKey;
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
    if( keyboardApi.JustPressed( keyMap.mKey ) )
      inputData->OnKeyPressed( keyMap.mTextInputKey );

  const Span< Codepoint > codepoints{ keyboardApi.GetCodepoints() };
  for( Codepoint codepoint : codepoints )
    inputData->OnCodepoint( codepoint );

  if( keyboardApi.JustPressed( Key::MouseLeft ) )
  {
    const int numGlyphsBeforeCaret = GetCaret( inputData->mCodepoints,
                                               mousePos.x - textPos.x );
    inputData->OnClick( numGlyphsBeforeCaret );
  }
  else if( keyboardApi.IsPressed( Key::MouseLeft ) )
  {
    const int numGlyphsBeforeCaret = GetCaret( inputData->mCodepoints,
                                               mousePos.x - textPos.x );
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
    const UI2DDrawData::Box box
    {
      .mMini  { textPos + v2( minCaretPos, 0 ) },
      .mMaxi  { textPos + v2( maxCaretPos, fontSize ) },
      .mColor { ImGuiGetColor( ImGuiCol::TextSelection ) },
    };
    drawData->AddBox( box, clipRect );
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
    const UI2DDrawData::Box box
    {
      .mMini { textPos + v2( caretPos - caretHalfWidth, caretYPadding ) },
      .mMaxi { textPos + v2( caretPos + caretHalfWidth, fontSize - caretYPadding ) },
      .mColor { v4( ImGuiGetColor( ImGuiCol::Text ).xyz(), a ) },
    };
    drawData->AddBox( box, clipRect );
  }
}



#if 0
static void ImGuiRenderWindow( ImGuiWindow* window, Errors& errors )
{
  // The child draw calls are stored in mParent->mDrawdata
  if( window->mParent )
    return;

  TAC_RENDER_GROUP_BLOCK( String() + __FUNCTION__ "(" + window->mName + ")" );

  const WindowHandle hDesktopWindow{ window->mWindowHandle };

  TAC_ASSERT( hDesktopWindow.IsValid() );
  const DesktopWindowState* desktopWindowState{ GetDesktopWindowState( hDesktopWindow ) };
  if( !desktopWindowState->mNativeWindowHandle )
    return;

  const v2 size{ desktopWindowState->GetSizeV2() };
  const int w{ desktopWindowState->mWidth };
  const int h{ desktopWindowState->mHeight };

  const Render::ViewHandle viewHandle{ WindowGraphicsGetView( hDesktopWindow ) };
  if( !viewHandle.IsValid() )
    return;

  if( window->mWindowHandleOwned )
  {
    const Render::FramebufferHandle hFB{ WindowGraphicsGetFramebuffer( hDesktopWindow ) };

    Render::SetViewFramebuffer( viewHandle, hFB );
    Render::SetViewport( viewHandle, Render::Viewport( size ) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect( size ) );
  }

  //window->mDrawData->DrawToTexture( viewHandle, w, h, errors );
  OS::OSDebugBreak();
}
#endif





#if 0
static void ImGuiRender( Errors& errors )
{
  //ImGuiGlobals::Instance.mDesktopWindows;
  //WindowDraws sWindowDraws;
  //sWindowDraws.Clear();

  // Describes the draw requests for a desktop window

  for( ImGuiDesktopWindowImpl* imguiDesktopWindow : ImGuiGlobals::Instance.mDesktopWindows )
  {
    TAC_CALL( imguiDesktopWindow->Render( errors ) );
  }

  //for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
  //  sWindowDraws.AddWindow( window );


  // this should iteratre through imgui views?
  // and the draw data shouldnt exist, it should just be inlined here
  // in 1 big ass vbo/ibo
  for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
    ImGuiRenderWindow( window, errors );

  //TAC_CALL( sWindowDraws.Render( errors ) );
}
#endif

Tac::ImGuiMouseCursor Tac::ImGuiGetMouseCursor()
{
  return ImGuiGlobals::Instance.mMouseCursor;
}

const char* Tac::ImGuiGetColName( const ImGuiCol colIdx )
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
    "ScrollbarBG",
    "Checkmark",
  };
  static_assert( TAC_ARRAY_SIZE( names ) == ( int )ImGuiCol::Count );
  return names[ ( int )colIdx ];
}

// -----------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------------------

//void ImGuiSetNextWindowPos( const v2 screenspacePos )
//{
//  gNextWindow.mScreenspacePos = screenspacePos;
//  gNextWindow.mScreenspacePosExists = true;
//}

void Tac::ImGuiSetNextWindowStretch()
{
  gNextWindow.mStretch = true;
}

void Tac::ImGuiSetNextWindowHandle( const WindowHandle& WindowHandle )
{
  gNextWindow.mWindowHandle = WindowHandle;
}

Tac::WindowHandle Tac::ImGuiGetWindowHandle()
{
  return ImGuiGlobals::Instance.mCurrentWindow->GetWindowHandle();
}

Tac::WindowHandle Tac::ImGuiGetWindowHandle( StringView name )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  ImGuiWindow* window{ globals.FindWindow( name ) };
  if( !window )
    return {};

  return window->mDesktopWindow->mWindowHandle;
}

Tac::ImGuiRect           Tac::ImGuiGetContentRect()
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  return window->mViewportSpaceVisibleRegion;
}

void Tac::ImGuiSetNextWindowMoveResize()            { gNextWindow.mMoveResize = true; }
void Tac::ImGuiSetNextWindowPosition( v2 position, ImGuiCondition cond )
{
  gNextWindow.mPosition = position;
  gNextWindow.mPositionValid = true;
}
void Tac::ImGuiSetNextWindowSize( v2 size, ImGuiCondition cond ) 
{
  gNextWindow.mSize = size;
  gNextWindow.mSizeValid = true;
}
void Tac::ImGuiSetNextWindowDisableBG()             { gNextWindow.mEnableBG = false; }

  Tac::ImGuiID      Tac::GetID( StringView label )
  {
    return ImGuiGlobals::Instance.mCurrentWindow->GetID( label );
  }

  void    Tac::PushID( StringView str )
  {
    ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
    ImGuiID id{ window->GetID( str ) };
    window->mIDStack.push_back( id );
  }

  void    Tac::PopID()
  {
    ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
    window->mIDStack.pop_back();
  }


// [ ] Q: imgui.begin should always be followed by a imgui.end,
//        regardless of the imgui.begin return value.
//        why is that?
bool Tac::ImGuiBegin( const StringView& name )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  SimWindowApi windowApi{ globals.mSimWindowApi };
  //ImGuiCreateWindow createWindowFn = globals.mCreateWindow;
  ImGuiWindow* window{ globals.FindWindow( name ) };
  if( !window )
  {
    WindowHandle hDesktopWindow{ gNextWindow.mWindowHandle };
    int desktopWindowWidth{};
    int desktopWindowHeight{};

    if( hDesktopWindow.IsValid() )
    {
      if( !windowApi.IsShown( hDesktopWindow ) )
        return false;

      const v2i size{ windowApi.GetSize( hDesktopWindow ) };
      desktopWindowWidth = size.x;
      desktopWindowHeight = size.y;
    }
    else
    {
      int x{ 50 };
      int y{ 50 };
      int w{ 800 };
      int h{ 600 };

      if( gNextWindow.mPositionValid )
      {
        x = ( int )gNextWindow.mPosition.x;
        y = ( int )gNextWindow.mPosition.y;
      }

      if( gNextWindow.mSizeValid )
      {
        w = ( int )gNextWindow.mSize.x;
        h = ( int )gNextWindow.mSize.y;
      }

      SettingsNode windowJson{ ImGuiGetWindowSettingsJson( name ) };
      x = ( int )windowJson.GetChild( "x" ).GetValueWithFallback( ( JsonNumber )x ).mNumber;
      y = ( int )windowJson.GetChild( "y" ).GetValueWithFallback( ( JsonNumber )y ).mNumber;
      w = ( int )windowJson.GetChild( "w" ).GetValueWithFallback( ( JsonNumber )w ).mNumber;
      h = ( int )windowJson.GetChild( "h" ).GetValueWithFallback( ( JsonNumber )h ).mNumber;

      //PlatformFns* platform = PlatformFns::GetInstance();
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
        .mPos{ pos },
        .mSize{ size }
      };

      hDesktopWindow = windowApi.CreateWindow( params );

      desktopWindowWidth = w;
      desktopWindowHeight = h;
    }

    v2 size{ gNextWindow.mSize };
    size.x += size.x > 0 ? 0 : desktopWindowWidth;
    size.y += size.y > 0 ? 0 : desktopWindowHeight;

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
    window->mDesktopWindow = imguiDesktopWindow;
    window->mWindowHandleOwned = !gNextWindow.mWindowHandle.IsValid();
    window->mViewportSpacePos = {};
    window->mSize = size;
    window->mStretchWindow = gNextWindow.mStretch;
    window->mMoveResizeWindow = gNextWindow.mMoveResize;
    window->mEnableBG = gNextWindow.mEnableBG;
    ImGuiGlobals::Instance.mAllWindows.push_back( window );
  }


  gNextWindow = {};

  TAC_ASSERT( window->mSize.x > 0 && window->mSize.y > 0 );

  ImGuiDesktopWindow* desktopWindow{ window->mDesktopWindow };
  WindowHandle hWindow = desktopWindow->mWindowHandle;
  window->mRequestTime = ImGuiGlobals::Instance.mElapsedSeconds;
  if( !windowApi.IsShown( hWindow ) )
    return false;

  ImGuiGlobals::Instance.mWindowStack.push_back( window );
  ImGuiGlobals::Instance.mCurrentWindow = window;

  if( window->mStretchWindow )
    window->mSize = windowApi.GetSize( hWindow );

  window->BeginFrame();

  return true;
}

void Tac::ImGuiEnd()
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };

  Vector< ImGuiWindow* >& windowStack{ globals.mWindowStack };

  windowStack.pop_back();
  globals.mCurrentWindow = windowStack.empty() ? nullptr : windowStack.back();
}

//bool ImGuiIsMouseHoveringRectScreenspace( ImGuiRect rectScreenspace )
//{
//  ImGuiGlobals& globals { ImGuiGlobals::Instance };
//  SimKeyboardApi keyboardApi { globals.mSimKeyboardApi };
//  const v2 screenspaceMousePos { keyboardApi.GetMousePosScreenspace() };
//  return rectScreenspace.ContainsPoint( screenspaceMousePos );
//}

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
    Instance.mAllWindows.push_back( child );
  }
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
  Instance.mWindowStack.pop_back();
  Instance.mCurrentWindow = Instance.mWindowStack.back();
}

void Tac::ImGuiBeginGroup()
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };

  window->PushXOffset();
  window->mCurrLineHeight = 0;

  const GroupData groupData
  {
    .mSavedCursorDrawPos { window->mViewportSpaceCurrCursor },
    .mSavedLineHeight { window->mCurrLineHeight },
  };
  window->mGroupSK.push_back( groupData );
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
  const UIStyle& style{ ImGuiGetStyle() };
  const v2 offset( style.itemSpacing.x, 0 );

  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  window->mViewportSpaceCurrCursor = window->mViewportSpacePrevCursor + offset;
  window->mCurrLineHeight = window->mPrevLineHeight;
}

//void ImGuiText( const char* utf8 )
//{
//  ImGuiText( StringView( utf8 ) );
//}

//void ImGuiText( const String& utf8 )
//{
//  ImGuiText( StringView( utf8 ) );
//}

//void ImGuiText( const StringView& utf8 )

//void ImGuiText( const StringView& s ) { ImGuiText( s.c_str() ); }

//void ImGuiText( const String& s ) { ImGuiText( ( StringView )s ); }

//void ImGuiText( const char* utf8 )
void Tac::ImGuiText( const StringView& utf8 )
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  UI2DDrawData* drawData{ window->mDrawData };

  const float fontSize{ ImGuiGetFontSize() };
  const v2 textPos{ window->mViewportSpaceCurrCursor };
  const v2 textSize{ CalculateTextSize( utf8, fontSize ) };
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( textPos, textSize ) };

  window->ItemSize( textSize );
  if( !window->Overlaps( origRect ) )
    return;

  const ImGuiRect clipRect{ window->Clip( origRect ) };

  const UI2DDrawData::Text text
  {
    .mPos      { textPos },
    .mFontSize { fontSize },
    .mUtf8     { utf8 },
    .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
  };

  drawData->PushDebugGroup( "ImGuiText", utf8 );
  drawData->AddText( text, &clipRect );
  drawData->PopDebugGroup();
}


bool Tac::ImGuiInputText( const StringView& label, String& text )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };

  const float fontSize{ ImGuiGetFontSize() };
  const float buttonPadding{ ImGuiGetButtonPadding() };
  const v2& itemSpacing{ ImGuiGetItemSpacing() };

  const Timestamp mouseReleaseSeconds{ globals.mElapsedSeconds };

  ImGuiWindow* window{ globals.mCurrentWindow };

  const ImGuiID id{ window->GetID( label ) };
  const String oldText{ text };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  const int lineCount{ ComputeLineCount( text ) };
  const float width = window->GetRemainingWidth();
  const v2 totalSize( width, lineCount * fontSize );
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( pos, totalSize ) };

  window->ItemSize( totalSize );
  if( !window->Overlaps( origRect ) )
    return false;


  TextInputData* textInputData{ window->mTextInputData };

  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( "ImGuiInputText", label );
  TAC_ON_DESTRUCT( drawData->PopDebugGroup() );

  const ImGuiRect clipRect{ window->Clip( origRect ) };
  const ImGuiID oldWindowActiveId{ GetActiveID() };
  const bool hovered{ window->IsHovered( clipRect ) };
  if( hovered )
  {
    //static Timestamp mouseMovementConsummation;
    //Mouse::TryConsumeMouseMovement( &mouseMovementConsummation, TAC_STACK_FRAME );

    if( keyboardApi.JustPressed( Key::MouseLeft ) )
      SetActiveID( id, window );
  }


  const v2 textPos{ pos + v2( buttonPadding, 0 ) };
  const v2 textBackgroundMaxi{ pos + v2( totalSize.x * ( 2.0f / 3.0f ), totalSize.y ) };

  const UI2DDrawData::Box box
  {
    .mMini { pos },
    .mMaxi { textBackgroundMaxi },
    .mColor { GetFrameColor( hovered ) },
  };

  drawData->AddBox( box, &clipRect );

  if( oldWindowActiveId == id )
  {
    if( oldWindowActiveId != id ) // just became active
    {
      textInputData->SetText( text );
    }

    TextInputDataUpdateKeys( textInputData, window->GetMousePosViewport(), textPos );

    // handle double click
    static Timestamp lastMouseReleaseSeconds;
    static v2 lastMousePositionDesktopWindowspace;
    if( keyboardApi.JustDepressed( Key::MouseLeft ) &&
        hovered &&
        !textInputData->mCodepoints.empty() )
    {
      const v2 screenspaceMousePos{ keyboardApi.GetMousePosScreenspace() };
      const Timestamp elapsedSecs{ ImGuiGlobals::Instance.mElapsedSeconds };
      const TimestampDifference kDoubleClickSecs{ 0.5f };
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
    .mPos { textPos },
    .mFontSize { fontSize },
    .mUtf8 { text },
    .mColor { ImGuiGetColor( ImGuiCol::Text ) },
  };
  drawData->AddText( drawText, &clipRect );

  const UI2DDrawData::Text labelText
  {
    .mPos { v2( textBackgroundMaxi.x + itemSpacing.x, pos.y ) },
    .mFontSize { fontSize },
    .mUtf8 { label },
    .mColor { ImGuiGetColor( ImGuiCol::Text ) },
  };
  drawData->AddText( labelText, &clipRect );

  return oldText != text;
}

bool Tac::ImGuiSelectable( const StringView& str, bool selected )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };

  const float fontSize{ ImGuiGetFontSize() };

  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };

  const float remainingWidth{ window->GetRemainingWidth() };

  const v2 buttonPosViewport{ window->mViewportSpaceCurrCursor };
  const v2 buttonSize( remainingWidth, fontSize );

  window->ItemSize( buttonSize );
  const ImGuiID id{ window->GetID( str ) };
  const ImGuiRect origRect{ ImGuiRect::FromPosSize( buttonPosViewport, buttonSize ) };
  if( !window->Overlaps( origRect ) )
    return false;

  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( "ImGuiSelectable", str );
  TAC_ON_DESTRUCT( drawData->PopDebugGroup() );

  const ImGuiRect clipRectViewport{ window->Clip( origRect ) };
  const bool hovered{ window->IsHovered( clipRectViewport ) };
  const bool active{ globals.mActiveID == id };
  const bool clicked{ hovered && keyboardApi.JustPressed( Key::MouseLeft ) };
  if( clicked )
    SetActiveID( id, window );

  if( active && !keyboardApi.IsPressed( Key::MouseLeft ) )
    ClearActiveID();

  if( selected || hovered )
  {
    const UI2DDrawData::Box box
    {
      .mMini { buttonPosViewport },
      .mMaxi { buttonPosViewport + buttonSize },
      .mColor { GetFrameColor( hovered ) },
    };
    drawData->AddBox( box, &clipRectViewport );
  }

  const UI2DDrawData::Text text
  {
    .mPos { buttonPosViewport },
    .mFontSize { fontSize },
    .mUtf8 { str },
    .mColor { ImGuiGetColor( ImGuiCol::Text ) },
  };
  drawData->AddText( text, &clipRectViewport );
  return clicked;
}


bool Tac::ImGuiButton( const StringView& str )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };

  const float fontSize{ ImGuiGetFontSize() };
  const float buttonPadding{ ImGuiGetButtonPadding() };

  ImGuiWindow* window{ globals.mCurrentWindow };

  const v2 textSize{ CalculateTextSize( str, fontSize ) };
  const v2 buttonSize{ textSize + v2( 2 * buttonPadding, 0 ) };
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
    //static Timestamp d;
    //Mouse::TryConsumeMouseMovement( &d, TAC_STACK_FRAME );
  }

  const UI2DDrawData::Box box
  {
    .mMini { pos },
    .mMaxi { pos + buttonSize },
    .mColor{ GetFrameColor( hovered ) },
  };

  const UI2DDrawData::Text text
  {
    .mPos      { pos + v2( buttonPadding, 0 ) },
    .mFontSize { fontSize },
    .mUtf8     { str },
    .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
  };


  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( "Button", str );
  drawData->AddBox( box, &clipRect );
  drawData->AddText( text, &clipRect );
  drawData->PopDebugGroup();

  return hovered && keyboardApi.JustPressed( Key::MouseLeft );
}



bool Tac::ImGuiCheckbox( const StringView& str, bool* value )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };

  const bool oldValue{ *value };
  const float fontSize{ ImGuiGetFontSize() };
  const v2& itemSpacing{ ImGuiGetItemSpacing() };

  ImGuiWindow* window{ globals.mCurrentWindow };

  const v2 pos{ window->mViewportSpaceCurrCursor };
  const v2 textSize{ CalculateTextSize( str, fontSize ) };
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

  if( hovered && keyboardApi.JustPressed( lmb ) )
  {
    *value = !*value;
    //Mouse::ButtonSetIsDown( lmb, false );
  }

  const UI2DDrawData::Box box
  {
    .mMini  { pos },
    .mMaxi  { pos + boxSize },
    .mColor { GetFrameColor( hovered ) },
  };

  const UI2DDrawData::Text text
  {
    .mPos      { pos + v2( boxWidth + itemSpacing.x, 0 ) },
    .mFontSize { fontSize },
    .mUtf8     { str },
    .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
  };


  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( "Checkbox", str );
  drawData->AddBox( box, &clipRect );

  if( *value )
    ImGuiDrawCheckMark( pos, boxWidth );

  drawData->AddText( text, &clipRect );
  drawData->PopDebugGroup();

  return oldValue != *value;
}

Tac::v2 Tac::ImGuiGetCursorPos()
{
  return ImGuiGlobals::Instance.mCurrentWindow->mViewportSpaceCurrCursor;
}

void Tac::ImGuiSetCursorPos( const v2 local )
{
  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  window->mViewportSpaceCurrCursor = local;
}

Tac::UIStyle& Tac::ImGuiGetStyle()
{
  return ImGuiGlobals::Instance.mUIStyle;
}

const Tac::v4& Tac::ImGuiGetColor( ImGuiCol col )
{
  UIStyle& style{ ImGuiGetStyle() };
  return style.colors[ ( int )col ];
}

void Tac::ImGuiSetColor( ImGuiCol colidx, v4 rgba )
{
  UIStyle& style{ ImGuiGetStyle() };
  style.colors[ ( int )colidx ] = rgba;
}


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
  ImGuiSetColor( ImGuiCol::Scrollbar, v4( 100, 100, 100, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::ScrollbarActive, v4( 130, 130, 130, 128 ) / 255.0f );
  ImGuiSetColor( ImGuiCol::ScrollbarBG, v4( 50, 50, 50, 255 ) / 255.0f );
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

  const UI2DDrawData::Box box
  {
    .mMini          { pos },
    .mMaxi          { pos + size },
    .mColor         { color },
    .mTextureHandle { Render::TextureHandle( hTex ) },
  };

  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( "ImGuiImage", ToString( hTex ) );
  drawData->AddBox( box );
  drawData->PopDebugGroup();
}

Tac::UI2DDrawData* Tac::ImGuiGetDrawData()
{
  return ImGuiGlobals::Instance.mCurrentWindow->mDrawData;
}

// -----------------------------------------------------------------------------------------------

bool Tac::ImGuiDragFloat( const StringView& s, float* v ) { return ImGuiDragFloatN( s, v, 1 ); }
bool Tac::ImGuiDragFloat2( const StringView& s, float* v ) { return ImGuiDragFloatN( s, v, 2 ); }
bool Tac::ImGuiDragFloat3( const StringView& s, float* v ) { return ImGuiDragFloatN( s, v, 3 ); }
bool Tac::ImGuiDragFloat4( const StringView& s, float* v ) { return ImGuiDragFloatN( s, v, 4 ); }
bool Tac::ImGuiDragInt( const StringView& s, int* v ) { return ImGuiDragIntN( s, v, 1 ); }
bool Tac::ImGuiDragInt2( const StringView& s, int* v ) { return ImGuiDragIntN( s, v, 2 ); }
bool Tac::ImGuiDragInt3( const StringView& s, int* v ) { return ImGuiDragIntN( s, v, 3 ); }
bool Tac::ImGuiDragInt4( const StringView& s, int* v ) { return ImGuiDragIntN( s, v, 4 ); }

// -----------------------------------------------------------------------------------------------

bool Tac::ImGuiCollapsingHeader( const StringView& name, const ImGuiNodeFlags flags )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };

  const float fontSize{ ImGuiGetFontSize() };
  const float buttonPadding{ ImGuiGetButtonPadding() };

  ImGuiWindow* window{ globals.mCurrentWindow };

  const float width{ window->GetRemainingWidth() };
  const v2 pos{ window->mViewportSpaceCurrCursor };
  const v2 totalSize( width, fontSize );
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

  if( hovered && keyboardApi.JustPressed( Key::MouseLeft ) )
    isOpen = !isOpen;

  const UI2DDrawData::Box box
  {
    .mMini  { pos },
    .mMaxi  { pos + totalSize },
    .mColor { GetFrameColor( hovered ) },
  };

  UI2DDrawData* drawData{ window->mDrawData };
  drawData->PushDebugGroup( "ImGuiCollapsingHeader", name );
  drawData->AddBox( box, &clipRect );
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
  const float fontSize{ ImGuiGetFontSize() };
  const float buttonPadding{ ImGuiGetButtonPadding() };

  ImGuiWindow* window{ ImGuiGlobals::Instance.mCurrentWindow };
  UI2DDrawData* drawData{ window->mDrawData };
  TAC_ASSERT( !window->mIsAppendingToMenu );
  window->mIsAppendingToMenu = true;

  const UI2DDrawData::Box box
  {
    .mMini  {},
    .mMaxi  { v2( window->mSize.x, fontSize + buttonPadding * 2 ) },
    .mColor { v3( 69, 45, 83 ) / 255.0f, 1.0f },
  };
  drawData->AddBox( box );
}

//void ImGuiBeginMenu( const String& label )
//{
//  ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
//  Assert( window->mIsAppendingToMenu );
//}

//void ImGuiMenuItem( const String& label )
//{
//  ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
//  Assert( window->mIsAppendingToMenu );
//}

//void ImGuiEndMenu()
//{
//  ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
//  Assert( window->mIsAppendingToMenu );
//}

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

  if( ImGuiWindow * window{ globals.mMovingWindow } )
    window->UpdateMoveControls();

  if( globals.mSettingsDirty )
    for( ImGuiWindow* window : globals.mAllWindows )
        ImGuiSaveWindowSettings( window );
}

//static bool ImGuiDesktopWindowOwned( WindowHandle WindowHandle )
//{
//  for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
//    if( window->mWindowHandleOwned && window->mWindowHandle == WindowHandle )
//      return true;
//  return false;
//}

void Tac::ImGuiEndFrame( Errors& errors )
{
  TAC_PROFILE_BLOCK;

  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  const SimWindowApi windowApi{ globals.mSimWindowApi };
  const SimKeyboardApi keyboardApi{ globals.mSimKeyboardApi };

  const Timestamp curSeconds{ globals.mElapsedSeconds };

  for( const ImGuiWindow* window : globals.mWindowStack )
  {
    TAC_ASSERT_CRITICAL( String() + "Mismatched ImGuiBegin/ImGuiEnd for " + window->mName );
  }

  //ImGuiRender( errors );

  FrameMemoryVector< ImGuiWindow* > windowsToDeleteImGui;


  for( ImGuiWindow* window : globals.mAllWindows )
    window->BeginMoveControls();

  

  for( ImGuiWindow* window : globals.mAllWindows )
  {
    const TimestampDifference deletionWaitSeconds{ 0.1f };
    if( curSeconds > window->mRequestTime + deletionWaitSeconds && window->mWindowHandleOwned )
    {
      windowsToDeleteImGui.push_back( window );
      continue;
    }

    if( window->mMoveResizeWindow )
    {
      if( WindowHandle WindowHandle{ window->GetWindowHandle() };
          WindowHandle.IsValid() )
      {
        //OS::OSDebugBreak();
        // TODO: copy MoveControls and ResizeControls logic here
        //DesktopApp::GetInstance()->MoveControls( WindowHandle );
        //DesktopApp::GetInstance()->ResizeControls( WindowHandle );

        //v2 newPos = ;
        //v2 newSize = ;
        //ImGuiGlobals::Instance.mSetWindowPos( WindowHandle, newPos ); 
        //ImGuiGlobals::Instance.mSetWindowSize( WindowHandle, newSize ); 
      }
    }

  }

  for( ImGuiWindow* window : windowsToDeleteImGui )
  {
    // Destroy the desktop app window
    //DesktopApp::GetInstance()->DestroyWindow( window->GetWindowHandle() );
    windowApi.DestroyWindow( window->GetWindowHandle() );


    // Destroy the imgui window
    {
      Vector< ImGuiWindow* >& windows{ globals.mAllWindows };

      const int i = [ & ]()
        {
          const int n{ windows.size() };
          for( int i{}; i < n; ++i )
            if( windows[ i ] == window )
              return i;

          TAC_ASSERT_INVALID_CODE_PATH;
          return 0;
        }( );

        windows[ i ] = windows.back();
        windows.pop_back();
        TAC_DELETE window;
    }
  }
}

float     Tac::ImGuiGetFontSize()
{
  const UIStyle& style{ ImGuiGetStyle() };
  return style.fontSize;
}

const Tac::v2& Tac::ImGuiGetItemSpacing()
{
  const UIStyle& style{ ImGuiGetStyle() };
  return style.itemSpacing;
}

float     Tac::ImGuiGetButtonPadding()
{
  const UIStyle& style{ ImGuiGetStyle() };
  return style.buttonPadding;
}

void Tac::ImGuiInit( const ImGuiInitParams& params, Errors& errors )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  globals.mMaxGpuFrameCount = params.mMaxGpuFrameCount;
  globals.mSimWindowApi = params.mSimWindowApi;
  globals.mSimKeyboardApi = params.mSimKeyboardApi;
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

void Tac::ImGuiSetIsScrollbarEnabled( bool b )
{
  ImGuiGlobals::Instance.mScrollBarEnabled = b;
}


Tac::ImGuiSimFrameDraws Tac::ImGuiGetSimFrameDraws()
{
  Vector< ImGuiSimWindowDraws > allWindowDraws;
  Vector< ImGuiSimFrameDraws::WindowSizeData > windowSizeDatas;

  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  const SimWindowApi windowApi{ globals.mSimWindowApi };

  for( ImGuiDesktopWindowImpl* window : globals.mDesktopWindows )
  {
    ImGuiSimWindowDraws curWindowDraws { window->GetSimWindowDraws() };
    allWindowDraws.push_back( curWindowDraws );
  }

  //for( ImGuiWindow* window : globals.mAllWindows )
  for( ImGuiDesktopWindowImpl* window : globals.mDesktopWindows )
  {
    //if( window->mWindowHandleOwned )
    if( window->mRequestedPosition.HasValue() || window->mRequestedSize.HasValue() )
    {
      //const WindowHandle windowHandle{ window->mDesktopWindow->mWindowHandle };
      const WindowHandle windowHandle{ window->mWindowHandle };

      const v2i windowPosScreenspace{ windowApi.GetPos( windowHandle ) };
      const ImGuiSimFrameDraws::WindowSizeData windowSizeData
      {
        .mWindowHandle      { windowHandle },
        .mRequestedPosition { window->mRequestedPosition },
        .mRequestedSize     { window->mRequestedSize },
      };

      windowSizeDatas.push_back( windowSizeData );
    }
  }

  return ImGuiSimFrameDraws
  {
    .mWindowDraws     { allWindowDraws },
    .mWindowSizeDatas { windowSizeDatas },
    .mCursor          { globals.mMouseCursor },
  };
}

void Tac::ImGuiPlatformRender( ImGuiSysDrawParams params, Errors& errors )
{
  ImGuiPersistantPlatformData::Instance.UpdateAndRender( params, errors );
}

void Tac::ImGuiPlatformPresent( const SysWindowApi windowApi, Errors& errors )
{
  ImGuiGlobals& globals{ ImGuiGlobals::Instance };
  for( ImGuiWindow* window : globals.mAllWindows )
  {
    if( !window->mWindowHandleOwned )
      continue;

    const WindowHandle windowHandle{ window->mDesktopWindow->mWindowHandle };
    if( !windowApi.IsShown( windowHandle ) )
      continue;

    const Render::SwapChainHandle swapChain { windowApi.GetSwapChainHandle( windowHandle ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( renderDevice->Present( swapChain, errors ) );
  }
}

