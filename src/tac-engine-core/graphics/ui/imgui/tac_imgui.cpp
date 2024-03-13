#include "tac_imgui.h" // self-inc
#include "tac_imgui_drag.h"

#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_frame_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_settings.h"
#include "tac-rhi/ui/imgui/tac_imgui_state.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/tac_renderer_util.h"
#include "tac-rhi/ui/tac_text_edit.h"
#include "tac-rhi/ui/tac_ui_2d.h"
#include "tac-std-lib/input/tac_keyboard_input.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/profile/tac_profile.h"
#include "tac-std-lib/shell/tac_shell.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/system/tac_desktop_window.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
//#include "tac-rhi/tac_render.h" // ?

#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"

namespace Tac
{



  int GetCaret( const Vector< Codepoint >& codepoints,
                       float mousePos ) // mouse pos rel text top left corner
  {
    const float fontSize = ImGuiGetFontSize();
    const int codepointCount = codepoints.size();

    float runningTextWidth = 0;
    int numGlyphsBeforeCaret = 0;
    for( int i = 1; i <= codepointCount; ++i )
    {
      const v2 substringSize = CalculateTextSize( codepoints.begin(), i, fontSize );
      const float lastGlyphWidth = substringSize.x - runningTextWidth;
      const float lastGlyphMidpoint = runningTextWidth + lastGlyphWidth / 2;

      if( mousePos < lastGlyphMidpoint )
        break;

      runningTextWidth += lastGlyphWidth;
      numGlyphsBeforeCaret++;
    }
    return numGlyphsBeforeCaret;
  }


  void TextInputDataUpdateKeys( TextInputData* inputData,
                                       const v2& mousePos,
                                       const v2& textPos )
  {
    struct KeyMap
    {
      Keyboard::Key mKey;
      TextInputKey  mTextInputKey;
    };
    
    const KeyMap keyMaps[] =
    {
      { Keyboard::Key::LeftArrow, TextInputKey::LeftArrow },
      { Keyboard::Key::RightArrow, TextInputKey::RightArrow },
      { Keyboard::Key::Backspace, TextInputKey::Backspace },
      { Keyboard::Key::Delete, TextInputKey::Delete },
    };

    for( const KeyMap& keyMap : keyMaps )
      if( Keyboard::KeyboardIsKeyJustDown( keyMap.mKey ) )
        inputData->OnKeyPressed( keyMap.mTextInputKey );

    if( Keyboard::KeyboardGetWMCharPressedHax() )
      inputData->OnCodepoint( Keyboard::KeyboardGetWMCharPressedHax() );

    if( Mouse::ButtonIsDown( Mouse::Button::MouseLeft ) )
    {
      const int numGlyphsBeforeCaret = GetCaret( inputData->mCodepoints,
                                                 mousePos.x - textPos.x );
      if( Mouse::ButtonWasDown( Mouse::Button::MouseLeft ) )
        inputData->OnDrag( numGlyphsBeforeCaret );
      else
        inputData->OnClick( numGlyphsBeforeCaret );
    }
  }



  void TextInputDataDrawSelection( TextInputData* inputData,
                                          UI2DDrawData* drawData,
                                          const v2& textPos,
                                          const ImGuiRect* clipRect )
  {
    const float fontSize = ImGuiGetFontSize();
    const Codepoint* codepoints = inputData->mCodepoints.data();
    if( inputData->mCaretCount == 2 )
    {
      const int minCaret = inputData->GetMinCaret();
      const int maxCaret = inputData->GetMaxCaret();
      const float minCaretPos = CalculateTextSize( codepoints, minCaret, fontSize ).x;
      const float maxCaretPos = CalculateTextSize( codepoints, maxCaret, fontSize ).x;
      const UI2DDrawData::Box box =
      {
        .mMini = textPos + v2( minCaretPos, 0 ),
        .mMaxi = textPos + v2( maxCaretPos, fontSize ),
        .mColor = ImGuiGetColor( ImGuiCol::TextSelection ),
      };
      drawData->AddBox( box, clipRect );
    }

    if( inputData->mCaretCount == 1 )
    {
      const int codepointCount = inputData->mNumGlyphsBeforeCaret[ 0 ];
      const float caretPos = CalculateTextSize( codepoints, codepointCount, fontSize ).x;
      const float caretYPadding = 2.0f;
      const float caretHalfWidth = 0.5f;
      const float blinkySpeed = 2.25f;
      const double t = ImGuiGlobals::Instance.mElapsedSeconds;
      const float a = 1 - Pow( ( float )Cos( blinkySpeed * t ), 4 );
      const UI2DDrawData::Box box =
      {
        .mMini = textPos + v2( caretPos - caretHalfWidth, caretYPadding ),
        .mMaxi = textPos + v2( caretPos + caretHalfWidth, fontSize - caretYPadding ),
        .mColor = v4( ImGuiGetColor( ImGuiCol::Text ).xyz(), a ),
      };
      drawData->AddBox( box, clipRect );
    }
  }

  static const v4& GetFrameColor( const bool hovered )
  {
    const bool active = hovered && Mouse::ButtonIsDown( Mouse::Button::MouseLeft );
    const v4& boxColor = ImGuiGetColor( active ? ImGuiCol::FrameBGActive :
                                        hovered ? ImGuiCol::FrameBGHovered : ImGuiCol::FrameBG );
    return boxColor;
  }

  static Vector< UI2DDrawGpuInterface > sDrawInterfaces;


#if 0
  static void ImGuiRenderWindow( ImGuiWindow* window, Errors& errors )
  {
    // The child draw calls are stored in mParent->mDrawdata
    if( window->mParent )
      return;

    TAC_RENDER_GROUP_BLOCK( String() + __FUNCTION__ "(" + window->mName + ")" );

    const DesktopWindowHandle hDesktopWindow = window->mDesktopWindowHandle;

    TAC_ASSERT( hDesktopWindow.IsValid() );
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( hDesktopWindow );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    const v2 size = desktopWindowState->GetSizeV2();
    const int w = desktopWindowState->mWidth;
    const int h = desktopWindowState->mHeight;

    const Render::ViewHandle viewHandle = WindowGraphicsGetView( hDesktopWindow );
    if( !viewHandle.IsValid() )
      return;

    if( window->mDesktopWindowHandleOwned )
    {
      const Render::FramebufferHandle hFB = WindowGraphicsGetFramebuffer( hDesktopWindow );

      Render::SetViewFramebuffer( viewHandle, hFB );
      Render::SetViewport( viewHandle, Render::Viewport( size ) );
      Render::SetViewScissorRect( viewHandle, Render::ScissorRect( size ) );
    }

    //window->mDrawData->DrawToTexture( viewHandle, w, h, errors );
    OS::OSDebugBreak();
  }
#endif





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


#if 0
    // this should iteratre through imgui views?
    // and the draw data shouldnt exist, it should just be inlined here
    // in 1 big ass vbo/ibo
    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
      ImGuiRenderWindow( window, errors );
#endif

    //TAC_CALL( sWindowDraws.Render( errors ) );
  }


  const char* ImGuiGetColName( const ImGuiCol colIdx )
  {
    constexpr const char* names[] =
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

  UIStyle::UIStyle()
  {
     ImGuiDefaultColors();
  }



  ImGuiRect ImGuiRect::FromPosSize( v2 pos, v2 size )
  {
    return
    {
      .mMini = pos,
      .mMaxi = pos + size,
    };
  }

  ImGuiRect ImGuiRect::FromMinMax( v2 mini, v2 maxi )
  {
    return
    {
      .mMini = mini,
      .mMaxi = maxi,
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

  bool ImGuiRect::Overlaps( const ImGuiRect& r ) const
  {
    return
      r.mMini.x <= mMaxi.x &&
      r.mMaxi.x >= mMini.x &&
      r.mMini.y <= mMaxi.y &&
      r.mMaxi.y >= mMini.y;
  }

  //void ImGuiSetNextWindowPos( const v2 screenspacePos )
  //{
  //  gNextWindow.mScreenspacePos = screenspacePos;
  //  gNextWindow.mScreenspacePosExists = true;
  //}

  void ImGuiSetNextWindowStretch()
  {
    gNextWindow.mStretch = true;
  }
  void ImGuiSetNextWindowHandle( const DesktopWindowHandle& desktopWindowHandle )
  {
    gNextWindow.mDesktopWindowHandle = desktopWindowHandle;
  }

  DesktopWindowHandle ImGuiGetWindowHandle()
  {
    return ImGuiGlobals::Instance.mCurrentWindow->GetDesktopWindowHandle();
  }

  void ImGuiSetNextWindowMoveResize()            { gNextWindow.mMoveResize = true; }
  void ImGuiSetNextWindowPosition( v2 position ) { gNextWindow.mPosition = position; }
  void ImGuiSetNextWindowSize( v2 size )         { gNextWindow.mSize = size; }
  void ImGuiSetNextWindowDisableBG()             { gNextWindow.mEnableBG = false; }

  static Json* ImGuiGetWindowsSettingsJson()
  {
    return SettingsGetJson( "imgui.windows" );
  }

  static Json* ImGuiGetWindowSettingsJson( const StringView& name )
  {
    Json* windowsJson = ImGuiGetWindowsSettingsJson();
    for( Json* curWindowJson : windowsJson->mArrayElements )
      if( SettingsGetString( "name", "", curWindowJson ) == name )
        return curWindowJson;

    Json* windowJson = windowsJson->AddChild();
    SettingsSetString( "name", name, windowJson );
    return windowJson;
  }

  static void ImGuiLoadWindowSettings( const StringView& name, int* x, int* y, int* w, int* h )
  {
    Json* windowJson = ImGuiGetWindowSettingsJson( name );
    *x = ( int )SettingsGetNumber( "x", *x, windowJson );
    *y = ( int )SettingsGetNumber( "y", *y, windowJson );
    *w = ( int )SettingsGetNumber( "w", *w, windowJson );
    *h = ( int )SettingsGetNumber( "h", *h, windowJson );
  }

  static void ImGuiSaveWindowSettings( const StringView& name, int x, int y, int w, int h )
  {
    Json* windowJson = ImGuiGetWindowSettingsJson( name );
    SettingsSetString( "name", name, windowJson );
    SettingsSetNumber( "x", x, windowJson );
    SettingsSetNumber( "y", y, windowJson );
    SettingsSetNumber( "w", w, windowJson );
    SettingsSetNumber( "h", h, windowJson );
  }


  // [ ] Q: imgui.begin should always be followed by a imgui.end,
  //        regardless of the imgui.begin return value.
  //        why is that?
  bool ImGuiBegin( const StringView& name )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.FindWindow( name );
    if( !window )
    {
      DesktopWindowHandle hDesktopWindow = gNextWindow.mDesktopWindowHandle;
      int desktopWindowWidth = 0;
      int desktopWindowHeight = 0;

      if( hDesktopWindow.IsValid() )
      {
        const DesktopWindowState* desktopWindowState = GetDesktopWindowState( hDesktopWindow );
        TAC_ASSERT( desktopWindowState );
        if( !desktopWindowState->mNativeWindowHandle )
          return false;

        desktopWindowWidth = desktopWindowState->mWidth;
        desktopWindowHeight = desktopWindowState->mHeight;
      }
      else
      {
        int x = gNextWindow.mPosition.x ? ( int )gNextWindow.mPosition.x : 50;
        int y = gNextWindow.mPosition.y ? ( int )gNextWindow.mPosition.y : 50;
        int w = gNextWindow.mSize.x ? ( int )gNextWindow.mSize.x : 800;
        int h = gNextWindow.mSize.y ? ( int )gNextWindow.mSize.y : 600;

        ImGuiLoadWindowSettings( name, &x, &y, &w, &h );

        const DesktopAppCreateWindowParams createParams
        {
          .mName = name,
          .mX = x,
          .mY = y,
          .mWidth = w,
          .mHeight = h,
        };

        hDesktopWindow = DesktopApp::GetInstance()->CreateWindow(createParams);
        desktopWindowWidth = w;
        desktopWindowHeight = h;
      }

      v2 size = gNextWindow.mSize;
      size.x += size.x > 0 ? 0 : desktopWindowWidth;
      size.y += size.y > 0 ? 0 : desktopWindowHeight;

      ImGuiDesktopWindowImpl* imguiDesktopWindow
        = ImGuiGlobals::Instance.FindDesktopWindow( hDesktopWindow );
      if( !imguiDesktopWindow )
      {
        imguiDesktopWindow = TAC_NEW ImGuiDesktopWindowImpl;
        imguiDesktopWindow->mDesktopWindowHandle = hDesktopWindow;
        ImGuiGlobals::Instance.mDesktopWindows.push_back( imguiDesktopWindow );
      }

      TAC_ASSERT( hDesktopWindow.IsValid() );
      window = TAC_NEW ImGuiWindow;
      window->mName = name;
      window->mDrawData = TAC_NEW UI2DDrawData;
      window->mDesktopWindow = imguiDesktopWindow;
      window->mDesktopWindowHandleOwned = !gNextWindow.mDesktopWindowHandle.IsValid();
      window->mViewportSpacePos = {};
      window->mSize = size;
      window->mStretchWindow = gNextWindow.mStretch;
      window->mMoveResizeWindow = gNextWindow.mMoveResize;
      window->mEnableBG = gNextWindow.mEnableBG;
      ImGuiGlobals::Instance.mAllWindows.push_back( window );
    }

    gNextWindow = {};

    TAC_ASSERT( window->mSize.x > 0 && window->mSize.y > 0 );
    
    const DesktopWindowState* desktopWindowState = window->GetDesktopWindowState();
    if( !desktopWindowState || !desktopWindowState->mNativeWindowHandle )
      return false;

    ImGuiGlobals::Instance.mWindowStack.push_back( window );
    ImGuiGlobals::Instance.mCurrentWindow = window;

    window->mRequestTime = ImGuiGlobals::Instance.mElapsedSeconds;
    window->mSize = window->mStretchWindow ? desktopWindowState->GetSizeV2() : window->mSize;
    window->BeginFrame();

    return true;
  }

  void ImGuiEnd()
  {
    Vector< ImGuiWindow* >& windowStack = ImGuiGlobals::Instance.mWindowStack;

    windowStack.pop_back();
    ImGuiGlobals::Instance.mCurrentWindow = windowStack.empty() ? nullptr : windowStack.back();
  }


  bool ImGuiIsMouseHoveringRectScreenspace( ImGuiRect rectScreenspace )
  {
    const v2 screenspaceMousePos = Mouse::GetScreenspaceCursorPos();
    return rectScreenspace.ContainsPoint( screenspaceMousePos );
  }

  void ImGuiBeginChild( const StringView& name, const v2& size )
  {
    ImGuiGlobals& Instance = ImGuiGlobals::Instance;

    ImGuiWindow* parent = Instance.mCurrentWindow;
    ImGuiWindow* child = Instance.FindWindow( name );
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

  void ImGuiEndChild()
  {
    ImGuiGlobals& Instance = ImGuiGlobals::Instance;

    ImGuiWindow* child = Instance.mCurrentWindow;
    child->mParent->ItemSize( child->mSize );
    Instance.mWindowStack.pop_back();
    Instance.mCurrentWindow = Instance.mWindowStack.back();
  }

  void ImGuiBeginGroup()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;

    window->PushXOffset();
    window->mCurrLineHeight = 0;

    const GroupData groupData =
    {
      .mSavedCursorDrawPos = window->mViewportSpaceCurrCursor,
      .mSavedLineHeight = window->mCurrLineHeight,
    };
    window->mGroupSK.push_back( groupData );
  }

  void ImGuiEndGroup()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    const GroupData& groupData = window->mGroupSK.back();
    const v2 groupSize = window->mViewportSpaceMaxiCursor - groupData.mSavedCursorDrawPos;

    window->mXOffsets.pop_back();
    window->mCurrLineHeight = groupData.mSavedLineHeight;
    window->mViewportSpaceCurrCursor = groupData.mSavedCursorDrawPos;
    window->ItemSize( groupSize );
    window->mGroupSK.pop_back();
  }

  void ImGuiIndent()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    window->mViewportSpaceCurrCursor.x += 15.0f;
    window->PushXOffset();
  }

  void ImGuiUnindent()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    window->mXOffsets.pop_back();
    window->mViewportSpaceCurrCursor.x = window->mViewportSpacePos.x + window->mXOffsets.back();
  }

  void ImGuiSameLine()
  {
    const UIStyle& style = ImGuiGetStyle();
    const v2 offset( style.itemSpacing.x, 0 );

    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
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
  void ImGuiText( const StringView& utf8 )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData;

    const float fontSize = ImGuiGetFontSize();
    const v2 textPos = window->mViewportSpaceCurrCursor;
    const v2 textSize = CalculateTextSize( utf8, fontSize );
    const ImGuiRect origRect = ImGuiRect::FromPosSize( textPos, textSize );

    window->ItemSize( textSize );
    if( !window->Overlaps( origRect ) )
      return;

    const ImGuiRect clipRect = window->Clip( origRect );

    const UI2DDrawData::Text text =
    {
      .mPos = textPos,
      .mFontSize = fontSize,
      .mUtf8 = utf8,
      .mColor = ImGuiGetColor( ImGuiCol::Text ),
    };

    drawData->PushDebugGroup( "ImGuiText", utf8 );
    drawData->AddText( text, &clipRect );
    drawData->PopDebugGroup();
  }

  static int ComputeLineCount( const StringView& s )
  {
    // todo: word wrap
    int lineCount = 1;
    for( char c : s )
      if( c == '\n' )
        lineCount++;
    return lineCount;
  }

  bool ImGuiInputText( const StringView& label, String& text )
  {
    const float fontSize = ImGuiGetFontSize();
    const float buttonPadding = ImGuiGetButtonPadding();
    const v2& itemSpacing = ImGuiGetItemSpacing();

    const Timestamp mouseReleaseSeconds = ImGuiGlobals::Instance.mElapsedSeconds;

    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;

    const ImGuiId id = window->GetID();
    const String oldText = text;
    const v2 pos = window->mViewportSpaceCurrCursor;
    const int lineCount = ComputeLineCount( text );
    const float width = window->GetRemainingWidth();
    const v2 totalSize( width, lineCount * fontSize );
    const ImGuiRect origRect = ImGuiRect::FromPosSize( pos, totalSize );

    window->ItemSize( totalSize );
    if( !window->Overlaps( origRect ) )
      return false;


    TextInputData* textInputData = window->mTextInputData;

    UI2DDrawData* drawData = window->mDrawData;
    drawData->PushDebugGroup("ImGuiInputText", label);
    TAC_ON_DESTRUCT( drawData->PopDebugGroup() );

    const ImGuiRect clipRect = window->Clip( origRect );
    const ImGuiId oldWindowActiveId = window->GetActiveID();
    const bool hovered = window->IsHovered( clipRect );
    if( hovered )
    {
      static Timestamp mouseMovementConsummation;
      Mouse::TryConsumeMouseMovement( &mouseMovementConsummation, TAC_STACK_FRAME );

      if( Mouse::ButtonJustDown( Mouse::Button::MouseLeft ) )
        window->SetActiveID( id );
    }


    const v2 textPos = pos + v2( buttonPadding, 0 );
    const v2 textBackgroundMaxi = pos + v2( totalSize.x * ( 2.0f / 3.0f ),
                                            totalSize.y );

    const UI2DDrawData::Box box =
    {
      .mMini = pos,
      .mMaxi = textBackgroundMaxi,
      .mColor = GetFrameColor( hovered ),
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
      if( Mouse::ButtonJustBeenReleased ( Mouse::Button::MouseLeft ) &&
          hovered &&
          !textInputData->mCodepoints.empty() )
      {
        const v2 screenspaceMousePos = Mouse::GetScreenspaceCursorPos();
        const Timestamp elapsedSecs = ImGuiGlobals::Instance.mElapsedSeconds;
        const TimestampDifference kDoubleClickSecs = 0.5f;
        const bool releasedRecently = elapsedSecs - lastMouseReleaseSeconds < kDoubleClickSecs;
        const bool releasedSamePos = lastMousePositionDesktopWindowspace == screenspaceMousePos;
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

    const UI2DDrawData::Text drawText =
    {
      .mPos = textPos,
      .mFontSize = fontSize,
      .mUtf8 = text,
      .mColor = ImGuiGetColor( ImGuiCol::Text ),
    };
    drawData->AddText( drawText, &clipRect );

    const UI2DDrawData::Text labelText =
    {
      .mPos = v2 ( textBackgroundMaxi.x + itemSpacing.x, pos.y ),
      .mFontSize = fontSize,
      .mUtf8 = label,
      .mColor = ImGuiGetColor( ImGuiCol::Text ),
    };
    drawData->AddText( labelText, &clipRect );

    return oldText != text;
  }

  bool ImGuiSelectable( const StringView& str, bool selected )
  {
    const float fontSize = ImGuiGetFontSize();

    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;

    const float remainingWidth = window->GetRemainingWidth();

    const v2 buttonPosViewport = window->mViewportSpaceCurrCursor;
    const v2 buttonSize( remainingWidth, fontSize );

    window->ItemSize( buttonSize );
    const ImGuiId id = window->GetID();
    const ImGuiRect origRect = ImGuiRect::FromPosSize( buttonPosViewport, buttonSize );
    if( !window->Overlaps( origRect ) )
      return false;


    UI2DDrawData* drawData = window->mDrawData;
    drawData->PushDebugGroup( "ImGuiSelectable", str );
    TAC_ON_DESTRUCT(drawData->PopDebugGroup());

    const ImGuiRect clipRectViewport = window->Clip( origRect );
    const bool hovered = window->IsHovered( clipRectViewport );
    const bool clicked = hovered && Mouse::ButtonJustDown( Mouse::Button::MouseLeft );
    if(clicked)
      window->SetActiveID( id );

    if( selected || hovered )
    {
      const UI2DDrawData::Box box =
      {
        .mMini = buttonPosViewport,
        .mMaxi = buttonPosViewport + buttonSize,
        .mColor = GetFrameColor( hovered ),
      };
      drawData->AddBox( box, &clipRectViewport );
    }

    const UI2DDrawData::Text text =
    {
      .mPos = buttonPosViewport,
      .mFontSize = fontSize,
      .mUtf8 = str,
      .mColor = ImGuiGetColor( ImGuiCol::Text ),
    };
    drawData->AddText( text, &clipRectViewport );
    return clicked;
  }


  bool ImGuiButton( const StringView& str )
  {
    const float fontSize = ImGuiGetFontSize();
    const float buttonPadding = ImGuiGetButtonPadding();

    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;

    const v2 textSize = CalculateTextSize( str, fontSize );
    const v2 buttonSize = textSize + v2( 2 * buttonPadding, 0 );
    const v2 pos = window->mViewportSpaceCurrCursor;

    window->ItemSize( textSize );

    // TODO: compare the various window->clip... apis against what dearimgui does
    const ImGuiRect origRect = ImGuiRect::FromPosSize( pos, buttonSize );
    if( !window->Overlaps( origRect ) )
      return false;


    const ImGuiRect clipRect = window->Clip( origRect );
    const bool hovered = window->IsHovered( clipRect );

    if( hovered )
    {
      static Timestamp d;
      Mouse::TryConsumeMouseMovement( &d, TAC_STACK_FRAME );
    }

    const UI2DDrawData::Box box =
    {
      .mMini = pos,
      .mMaxi = pos + buttonSize,
      .mColor = GetFrameColor( hovered ),
    };

    const UI2DDrawData::Text text =
    {
      .mPos = pos + v2( buttonPadding, 0 ),
      .mFontSize = fontSize,
      .mUtf8 = str,
      .mColor = ImGuiGetColor( ImGuiCol::Text ),
    };


    UI2DDrawData* drawData = window->mDrawData;
    drawData->PushDebugGroup( "Button", str );
    drawData->AddBox( box, &clipRect );
    drawData->AddText( text, &clipRect );
    drawData->PopDebugGroup();

    return hovered && Mouse::ButtonJustDown( Mouse::Button::MouseLeft );
  }

  static void ImGuiDrawCheckMark( const v2& pos, const  float boxWidth )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData;

    const v4& checkmarkColor = ImGuiGetColor( ImGuiCol::Checkmark );

    // (0,0)-------+
    // |         2 |
    // |        /  |
    // | 0     /   |
    // |   \  /    |
    // |     1     |
    // +-------(1,1)
    v2 ps[] = { v2( 0.2f, 0.4f ), v2( 0.45f, 0.90f ), v2( 0.9f, 0.1f ) };
    for( v2& p : ps )
      p = p * boxWidth + pos;

    const float lineRadius = boxWidth * 0.05f;

    const UI2DDrawData::Line firstLine =
    {
      .mP0 = ps[ 0 ],
      .mP1 = ps[ 1 ],
      .mLineRadius = lineRadius,
      .mColor = checkmarkColor,
    };

    const UI2DDrawData::Line secondLine =
    {
      .mP0 = ps[ 1 ],
      .mP1 = ps[ 2 ],
      .mLineRadius = lineRadius,
      .mColor = checkmarkColor,
    };

    drawData->PushDebugGroup("Checkmark");
    drawData->AddLine( firstLine );
    drawData->AddLine( secondLine );
    drawData->PopDebugGroup();
  }

  bool ImGuiCheckbox( const StringView& str, bool* value )
  {
    const bool oldValue = *value;
    const float fontSize = ImGuiGetFontSize();
    const v2& itemSpacing = ImGuiGetItemSpacing();

    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;

    const v2 pos = window->mViewportSpaceCurrCursor;
    const v2 textSize = CalculateTextSize( str, fontSize );
    const float boxWidth = textSize.y;
    const v2 boxSize = v2( 1, 1 ) * boxWidth;
    const v2 totalSize = textSize + v2( boxWidth + itemSpacing.x, 0 );
    const ImGuiRect origRect = ImGuiRect::FromPosSize( pos, totalSize );

    window->ItemSize( totalSize );
    if( !window->Overlaps(origRect ) )
      return false;

    const ImGuiRect clipRect = window->Clip(origRect);
    const bool hovered = window->IsHovered( clipRect );
    const Mouse::Button lmb = Mouse::Button::MouseLeft;

    if( hovered && Mouse::ButtonJustDown( lmb ) )
    {
      *value = !*value;
      Mouse::ButtonSetIsDown( lmb, false );
    }

    const UI2DDrawData::Box box =
    {
      .mMini = pos,
      .mMaxi = pos + boxSize,
      .mColor = GetFrameColor( hovered ),
    };

    const UI2DDrawData::Text text =
    {
      .mPos = pos + v2( boxWidth + itemSpacing.x, 0 ),
      .mFontSize = fontSize,
      .mUtf8 = str,
      .mColor = ImGuiGetColor( ImGuiCol::Text ),
    };


    UI2DDrawData* drawData = window->mDrawData;
    drawData->PushDebugGroup("Checkbox", str );
    drawData->AddBox( box, &clipRect );

    if( *value )
      ImGuiDrawCheckMark( pos, boxWidth);

    drawData->AddText( text, &clipRect );
    drawData->PopDebugGroup();
    
    return oldValue != *value;
  }

  v2 ImGuiGetCursorPos()
  {
    return ImGuiGlobals::Instance.mCurrentWindow->mViewportSpaceCurrCursor;
  }

  void ImGuiSetCursorPos( const v2 local )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    window->mViewportSpaceCurrCursor = local;
  }

  UIStyle& ImGuiGetStyle()
  {
    return ImGuiGlobals::Instance.mUIStyle;
  }

  const v4& ImGuiGetColor( ImGuiCol col )
  {
    UIStyle& style = ImGuiGetStyle();
    return style.colors[ (int)col ];
  }
  
  void ImGuiSetColor( ImGuiCol colidx, v4 rgba)
  {
    UIStyle& style = ImGuiGetStyle();
    style.colors[ (int)colidx ] = rgba;
  }


  void ImGuiDebugColors()
  {
    ImGuiDefaultColors();
    ImGuiSetColor( ImGuiCol::Text,                  v4( 255, 255, 255, 255 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::WindowBackground,      v4(  10,  20,  30, 255 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::ChildWindowBackground, v4(   5,  10,  15, 255 ) / 255.0f );
  }

  void ImGuiDefaultColors()
  {
    const v4 unsetColor( -1.0f );

    UIStyle& style = ImGuiGetStyle();
    for( v4& color : style.colors )
      color = unsetColor;

    ImGuiSetColor( ImGuiCol::Text,                  v4( 218, 218, 218, 255 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::TextSelection,         v4( 118, 178, 118, 155 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::WindowBackground,      v4(  29,  30,  32, 255 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::ChildWindowBackground, v4(  15,  15,  15, 255 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::FrameBG,               v4(  75, 104,  65, 128 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::FrameBGHovered,        v4( 115, 143,  91, 128 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::FrameBGActive,         v4( 128, 163,  85, 128 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::Scrollbar,             v4( 100, 100, 100, 128 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::ScrollbarActive,       v4( 130, 130, 130, 128 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::ScrollbarBG,           v4(  50,  50,  50, 255 ) / 255.0f );
    ImGuiSetColor( ImGuiCol::Checkmark,             v4( 249, 181,  53, 255 ) / 255.0f );

    for( v4& color : style.colors )
    {
      TAC_ASSERT( color != unsetColor );
    }
  }

  void ImGuiImage( const int hTex, const v2& size, const v4& color )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    const v2 pos = window->mViewportSpaceCurrCursor;

    window->ItemSize( size );

    const UI2DDrawData::Box box =
    {
      .mMini = pos,
      .mMaxi = pos + size,
      .mColor = color,
      .mTextureHandle = Render::TextureHandle( hTex ),
    };

    UI2DDrawData* drawData = window->mDrawData;
    drawData->PushDebugGroup( "ImGuiImage", ToString( hTex ) );
    drawData->AddBox( box );
    drawData->PopDebugGroup();
  }

  UI2DDrawData* ImGuiGetDrawData()
  {
    return ImGuiGlobals::Instance.mCurrentWindow->mDrawData;
  }

  // -----------------------------------------------------------------------------------------------

  bool ImGuiDragFloat( const StringView& s, float* v )  { return ImGuiDragFloatN( s, v, 1 ); }
  bool ImGuiDragFloat2( const StringView& s, float* v ) { return ImGuiDragFloatN( s, v, 2 ); }
  bool ImGuiDragFloat3( const StringView& s, float* v ) { return ImGuiDragFloatN( s, v, 3 ); }
  bool ImGuiDragFloat4( const StringView& s, float* v)  { return ImGuiDragFloatN( s, v, 4 ); }
  bool ImGuiDragInt( const StringView& s, int* v )      { return ImGuiDragIntN( s, v, 1 ); }
  bool ImGuiDragInt2( const StringView& s, int* v )     { return ImGuiDragIntN( s, v, 2 ); }
  bool ImGuiDragInt3( const StringView& s, int* v )     { return ImGuiDragIntN( s, v, 3 ); }
  bool ImGuiDragInt4( const StringView& s, int* v )     { return ImGuiDragIntN( s, v, 4 ); }

  // -----------------------------------------------------------------------------------------------

  bool ImGuiCollapsingHeader( const StringView& name, const ImGuiNodeFlags flags )
  {
    const float fontSize = ImGuiGetFontSize();
    const float buttonPadding = ImGuiGetButtonPadding();

    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;

    const float width = window->GetRemainingWidth();
    const v2 pos = window->mViewportSpaceCurrCursor;
    const v2 totalSize( width, fontSize );
    const ImGuiRect origRect = ImGuiRect::FromPosSize( pos, totalSize );

    window->ItemSize( totalSize );
    if( !window->Overlaps(origRect) )
      return false;

    const ImGuiRect clipRect = window->Clip( origRect );
    const bool hovered = window->IsHovered( clipRect );
    const ImGuiId id = window->GetID();

    if( flags & ImGuiNodeFlags_DefaultOpen && !window->mCollapsingHeaderStates.contains( id ) )
      window->mCollapsingHeaderStates[ id ] = true;

    bool& isOpen = window->mCollapsingHeaderStates[ id ];

    if( hovered && Mouse::ButtonJustDown( Mouse::Button::MouseLeft ) )
      isOpen = !isOpen;

    const UI2DDrawData::Box box =
    {
      .mMini = pos,
      .mMaxi = pos + totalSize,
      .mColor = GetFrameColor( hovered )
    };

    UI2DDrawData* drawData = window->mDrawData;
    drawData->PushDebugGroup( "ImGuiCollapsingHeader", name );
    drawData->AddBox( box, &clipRect );
    ImGuiSetCursorPos( pos + v2( buttonPadding, 0 ) );
    ImGuiText( isOpen ? "v" : ">" );
    ImGuiSameLine();
    ImGuiText( name );
    drawData->PopDebugGroup();

    return isOpen;
  }

  void ImGuiPushFontSize( float value )
  {
    ImGuiGlobals::Instance.mFontSizeSK.push_back( ImGuiGlobals::Instance.mUIStyle.fontSize );
    ImGuiGlobals::Instance.mUIStyle.fontSize = value;
  }

  void ImGuiPopFontSize()
  {
    ImGuiGlobals::Instance.mUIStyle.fontSize = ImGuiGlobals::Instance.mFontSizeSK.back();
    ImGuiGlobals::Instance.mFontSizeSK.pop_back();
  }

  void ImGuiBeginMenuBar()
  {
    const float fontSize = ImGuiGetFontSize();
    const float buttonPadding = ImGuiGetButtonPadding();

    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData;
    TAC_ASSERT( !window->mIsAppendingToMenu );
    window->mIsAppendingToMenu = true;
    const UI2DDrawData::Box box =
    {
      .mMini = {},
      .mMaxi = v2( window->mSize.x, fontSize + buttonPadding * 2 ),
      .mColor = { v3( 69, 45, 83 ) / 255.0f, 1.0f },
    };
    drawData->AddBox(box);
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

  void ImGuiEndMenuBar()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    TAC_ASSERT( window->mIsAppendingToMenu );
    window->mIsAppendingToMenu = false;
  }

  void ImGuiDebugDraw()
  {
    const ImGuiId id = ImGuiGlobals::Instance.mCurrentWindow->GetActiveID();
    ImGuiText( ShortFixedString::Concat( "Cur window active id: ", ToString( id ) ) );
  }



  void ImGuiBeginFrame(const BeginFrameData& data )
  {
    ImGuiGlobals::Instance.mElapsedSeconds = data.mElapsedSeconds;
    ImGuiGlobals::Instance.mMouseHoveredWindow = data.mMouseHoveredWindow;
  }

  //static bool ImGuiDesktopWindowOwned( DesktopWindowHandle desktopWindowHandle )
  //{
  //  for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
  //    if( window->mDesktopWindowHandleOwned && window->mDesktopWindowHandle == desktopWindowHandle )
  //      return true;
  //  return false;
  //}

  void ImGuiEndFrame( Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    for( const ImGuiWindow* window : ImGuiGlobals::Instance.mWindowStack )
    {
      TAC_ASSERT_CRITICAL( String() + "Mismatched ImGuiBegin/ImGuiEnd for " + window->mName );
    }

    ImGuiRender( errors );

    const Timestamp curSeconds = ImGuiGlobals::Instance.mElapsedSeconds;
    FrameMemoryVector< ImGuiWindow* > windowsToDeleteImGui;

    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
    {
      const TimestampDifference deletionWaitSeconds = 0.1f;
      if( curSeconds > window->mRequestTime + deletionWaitSeconds && window->mDesktopWindowHandleOwned )
      {
        windowsToDeleteImGui.push_back( window );
        continue;
      }

      if( window->mMoveResizeWindow )
      {
        if( DesktopWindowHandle desktopWindowHandle = window->GetDesktopWindowHandle();
            desktopWindowHandle.IsValid() )
        {
          DesktopApp::GetInstance()->MoveControls( desktopWindowHandle );
          DesktopApp::GetInstance()->ResizeControls( desktopWindowHandle );
        }
      }

    }

    for( ImGuiWindow* window : windowsToDeleteImGui )
    {
      // Destroy the desktop app window
      DesktopApp::GetInstance()->DestroyWindow( window->GetDesktopWindowHandle() );

      // Destroy the imgui window
      {
        Vector< ImGuiWindow* >& windows = ImGuiGlobals::Instance.mAllWindows;

        // Get the window index
        const int n = windows.size();
        int i = n;
        for( i = 0; i < n; ++i )
          if( windows[ i ] == window )
            break;
        TAC_ASSERT_INDEX( i, n );

        windows[ i ] = windows.back();
        windows.pop_back();
        TAC_DELETE window;
      }
    }
  }

  float     ImGuiGetFontSize()
  {
    const UIStyle& style = ImGuiGetStyle();
    return style.fontSize;
  }

  const v2& ImGuiGetItemSpacing()
  {
    const UIStyle& style = ImGuiGetStyle();
    return style.itemSpacing;
  }

  float     ImGuiGetButtonPadding()
  {
    const UIStyle& style = ImGuiGetStyle();
    return style.buttonPadding;
  }

  void ImGuiInit(int maxGPUFrameCount )
  {
    ImGuiGlobals::Instance.mMaxGpuFrameCount = maxGPUFrameCount;
  }

  void ImGuiSaveWindowSettings()
  {
    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
      if( window->mDesktopWindowHandleOwned )
        if( const DesktopWindowState* desktopWindowState = window->GetDesktopWindowState() )
          ImGuiSaveWindowSettings( window->mName,
                                   desktopWindowState->mX,
                                   desktopWindowState->mY,
                                   desktopWindowState->mWidth,
                                   desktopWindowState->mHeight );
  }

  void ImGuiUninit()
  {
    ImGuiSaveWindowSettings();
  }

  void ImGuiSetIsScrollbarEnabled( bool b)
  {
    ImGuiGlobals::Instance.mScrollBarEnabled = b;
  }

} // namespace Tac

