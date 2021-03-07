#include "src/common/math/tacVector4.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacSettings.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/imgui/tacImGuiState.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/graphics/tacTextEdit.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacShell.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/shell/tacDesktopWindowGraphics.h"
#include "src/shell/tacDesktopApp.h"

#include <cstdlib> // atof
#include <iostream> // cout

namespace Tac
{
  static void DebugCoutVec( const char* name, v2 v )
  {
    std::cout << name << "(" << v.x << ", " << v.y << ")" << std::endl;
  }

  static int GetCaret( // UI2DDrawData* drawData,
                       const Vector< Codepoint >& codepoints,
                       float mousePos ) // mouse pos rel text top left corner
  {
    auto codepointCount = ( int )codepoints.size();
    float runningTextWidth = 0;
    int numGlyphsBeforeCaret = 0;
    for( int i = 1; i <= codepointCount; ++i )
    {
      const v2 substringSize = CalculateTextSize( codepoints.begin(),
                                                  i,
                                                  ImGuiGlobals::Instance.mUIStyle.fontSize );

      const float lastGlyphWidth = substringSize.x - runningTextWidth;
      const float lastGlyphMidpoint = runningTextWidth + lastGlyphWidth / 2;

      if( mousePos < lastGlyphMidpoint )
        break;

      runningTextWidth += lastGlyphWidth;
      numGlyphsBeforeCaret++;
    }
    return numGlyphsBeforeCaret;
  }


  static void TextInputDataUpdateKeys( TextInputData* inputData, v2 textPos )
  {
    // UI2DDrawData* drawData = window->mDrawData;
    struct
    {
      Key mKey;
      TextInputKey mTextInputKey;
    } keyMaps[] =
    {
      { Key::LeftArrow, TextInputKey::LeftArrow },
      { Key::RightArrow, TextInputKey::RightArrow },
      { Key::Backspace, TextInputKey::Backspace },
      { Key::Delete, TextInputKey::Delete },
    };
    for( auto keyMap : keyMaps )
      if( gKeyboardInput.IsKeyJustDown( keyMap.mKey ) )
        inputData->OnKeyPressed( keyMap.mTextInputKey );
    if( gKeyboardInput.mWMCharPressedHax )
      inputData->OnCodepoint( gKeyboardInput.mWMCharPressedHax );
    if( gKeyboardInput.mCurr.IsKeyDown( Key::MouseLeft ) &&
        gKeyboardInput.mCurr.mScreenspaceCursorPosErrors.empty() )
    {
      const float mousePositionTextSpace = gKeyboardInput.mCurr.mScreenspaceCursorPos.x - textPos.x;
      const int numGlyphsBeforeCaret = GetCaret( inputData->mCodepoints,
                                                 mousePositionTextSpace );
      if( gKeyboardInput.mPrev.IsKeyDown( Key::MouseLeft ) )
        inputData->OnDrag( numGlyphsBeforeCaret );
      else
        inputData->OnClick( numGlyphsBeforeCaret );
    }
  }


  static void TextInputDataDrawSelection( TextInputData* inputData,
                                          UI2DDrawData* drawData,
                                          const v2 textPos,
                                          const ImGuiRect* clipRect )
  {
    if( inputData->mCaretCount == 2 )
    {
      const float minCaretPos = CalculateTextSize( inputData->mCodepoints.data(),
                                                   inputData->GetMinCaret(),
                                                   ImGuiGlobals::Instance.mUIStyle.fontSize ).x;

      const float maxCaretPos = CalculateTextSize( inputData->mCodepoints.data(),
                                                   inputData->GetMaxCaret(),
                                                   ImGuiGlobals::Instance.mUIStyle.fontSize ).x;

      const v2 selectionMini = v2( textPos.x + minCaretPos,
                                   textPos.y );
      const v2 selectionMaxi = v2( textPos.x + maxCaretPos,
                                   textPos.y + ImGuiGlobals::Instance.mUIStyle.fontSize );

      drawData->AddBox( selectionMini,
                        selectionMaxi,
                        { 0.5f, 0.5f, 0.0f, 1 },
                        Render::TextureHandle(),
                        clipRect );
    }

    if( inputData->mCaretCount == 1 )
    {
      const float caretPos = CalculateTextSize( inputData->mCodepoints.data(),
                                                inputData->mNumGlyphsBeforeCaret[ 0 ],
                                                ImGuiGlobals::Instance.mUIStyle.fontSize ).x;
      const float caretYPadding = 2.0f;
      const float caretHalfWidth = 0.5f;
      const v2 caretMini = v2( textPos.x + caretPos - caretHalfWidth,
                               textPos.y + caretYPadding );
      const v2 caretMaxi = v2( textPos.x + caretPos + caretHalfWidth,
                               textPos.y + ImGuiGlobals::Instance.mUIStyle.fontSize - caretYPadding );
      const float caretColorAlpha = ( ( std::sin( 6.0f * ( float )ImGuiGlobals::Instance.mElapsedSeconds ) + 1.0f ) / 2.0f );
      const v4 caretColor = { 0, 0, 0, caretColorAlpha };
      Render::TextureHandle texture;
      drawData->AddBox( caretMini, caretMaxi, caretColor, texture, clipRect );
    }
  }

  static bool ImguiDragVal( StringView str,
                            void* valueBytes,
                            int valueByteCount,
                            void( *valueToStringGetter )( String& to, const void* from ),
                            void( *valueFromStringSetter )( StringView from, void* to ),
                            void( *whatToDoWithMousePixel )( float mouseChangeSinceBeginningOfDrag, const void* valAtDragStart, void* curVal ) )
  {

    v4 backgroundBoxColor = { 1, 1, 0, 1 };
    String valueStr;
    valueToStringGetter( valueStr, valueBytes );

    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData;// ImGuiGlobals::Instance.mUI2DDrawData;
    TextInputData* inputData = window->mTextInputData;
    v2 pos = window->mCurrCursorViewport;
    v2 totalSize = {
      window->mContentRect.mMaxi.x - pos.x,
      ( float )ImGuiGlobals::Instance.mUIStyle.fontSize };
    window->ItemSize( totalSize );
    ImGuiId id = window->GetID();
    bool clipped;
    ImGuiRect clipRect = ImGuiRect::FromPosSize( pos, totalSize );
    window->ComputeClipInfo( &clipped, &clipRect );
    if( clipped )
      return false;

    DragData& dragFloatData = window->mDragDatas[ id ];

    // Only used to check if this function should return true/false because the value
    // changed or didnt change
    static Vector< char > valueFrameCopy;
    valueFrameCopy.resize( valueByteCount );
    MemCpy( valueFrameCopy.data(), valueBytes, valueByteCount );
    v2 valuePos = {
      pos.x + ImGuiGlobals::Instance.mUIStyle.buttonPadding,
      pos.y };


    const bool hovered = window->IsHovered( clipRect );

    static double consumeMouse;
    if( hovered )
      gKeyboardInput.TryConsumeMouseMovement( &consumeMouse );

    if( hovered && gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
    {
      window->SetActiveID( id );
    }
    if( window->GetActiveID() == id )
    {
      if( dragFloatData.mMode == DragMode::Drag && gKeyboardInput.mCurr.mScreenspaceCursorPosErrors.empty() )
      {
        const v2 screenspaceMousePos = gKeyboardInput.mCurr.mScreenspaceCursorPos;
        static float lastMouseXDesktopWindowspace;
        if( hovered )
          backgroundBoxColor.xyz() /= 2.0f;
        if( gKeyboardInput.IsKeyDown( Key::MouseLeft ) )
        {
          backgroundBoxColor.xyz() /= 2.0f;
          if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
          {
            lastMouseXDesktopWindowspace = screenspaceMousePos.x;
            dragFloatData.mDragDistPx = 0;
            dragFloatData.mValueCopy.resize( valueByteCount );
            MemCpy( dragFloatData.mValueCopy.data(), valueBytes, valueByteCount );
          }

          if( gKeyboardInput.mPrev.IsKeyDown( Key::MouseLeft ) )
          {
            const DesktopWindowState* desktopWindowState = GetDesktopWindowState( window->mDesktopWindowHandle );
            float moveCursorDir = 0;
            if( screenspaceMousePos.x > clipRect.mMaxi.x + desktopWindowState->mX )
              moveCursorDir = -1.0f;
            if( screenspaceMousePos.x < clipRect.mMini.x + desktopWindowState->mX )
              moveCursorDir = 1.0f;
            if( moveCursorDir )
            {
              float xOffset = moveCursorDir * clipRect.GetWidth();
              OS::SetScreenspaceCursorPos( screenspaceMousePos + v2( xOffset, 0 ), Errors() );
            }
            else
            {
              dragFloatData.mDragDistPx += screenspaceMousePos.x - lastMouseXDesktopWindowspace;
              whatToDoWithMousePixel( dragFloatData.mDragDistPx, dragFloatData.mValueCopy.data(), valueBytes );
            }
          }
        }

        lastMouseXDesktopWindowspace = screenspaceMousePos.x;

        // handle double click
        static double lastMouseReleaseSeconds;
        static v2 lastMousePositionDesktopWindowspace;
        if( gKeyboardInput.HasKeyJustBeenReleased( Key::MouseLeft ) &&
            hovered )
        {
          auto mouseReleaseSeconds = ImGuiGlobals::Instance.mElapsedSeconds;
          if( mouseReleaseSeconds - lastMouseReleaseSeconds < 0.5f &&
              lastMousePositionDesktopWindowspace == screenspaceMousePos )
          {
            CodepointView codepoints = UTF8ToCodepoints( valueStr );
            inputData->SetCodepoints( codepoints );
            inputData->mCaretCount = 2;
            inputData->mNumGlyphsBeforeCaret[ 0 ] = 0;
            inputData->mNumGlyphsBeforeCaret[ 1 ] = codepoints.size();
            dragFloatData.mMode = DragMode::TextInput;
          }
          lastMouseReleaseSeconds = mouseReleaseSeconds;
          lastMousePositionDesktopWindowspace = screenspaceMousePos;
        }
        //if( !gKeyboardInput.IsKeyDown( Key::MouseLeft ) )
        //  window->mActiveID = ImGuiIdNull;
      }

      if( dragFloatData.mMode == DragMode::TextInput )
      {
        TextInputDataUpdateKeys( inputData, valuePos );

        const StringView newText = CodepointsToUTF8( CodepointView( inputData->mCodepoints.data(),
                                                                    inputData->mCodepoints.size() ) );
        valueFromStringSetter( newText, valueBytes );
        valueStr = newText;

        if( gKeyboardInput.IsKeyJustDown( Key::Tab ) )
          window->mIDAllocator->mActiveID++;
      }
    }

    if( dragFloatData.mMode == DragMode::TextInput && id != window->GetActiveID() )
    {
      dragFloatData.mMode = DragMode::Drag;
      dragFloatData.mDragDistPx = 0;
    }

    v2 backgroundBoxMaxi = {
      pos.x + totalSize.x * ( 2.0f / 3.0f ),
      pos.y + totalSize.y };
    Render::TextureHandle texture;
    drawData->AddBox( pos, backgroundBoxMaxi, backgroundBoxColor, texture, &clipRect );

    if( dragFloatData.mMode == DragMode::TextInput )
      TextInputDataDrawSelection( inputData, drawData, valuePos, &clipRect );
    drawData->AddText( valuePos, ImGuiGlobals::Instance.mUIStyle.fontSize, valueStr, v4( 0, 0, 0, 1 ), &clipRect );

    v2 labelPos = {
      backgroundBoxMaxi.x + ImGuiGlobals::Instance.mUIStyle.itemSpacing.x,
      pos.y };
    drawData->AddText( labelPos, ImGuiGlobals::Instance.mUIStyle.fontSize, str, ImGuiGlobals::Instance.mUIStyle.textColor, &clipRect );


    return MemCmp( valueFrameCopy.data(), valueBytes, valueByteCount );
  }


  ImGuiRect ImGuiRect::FromPosSize( v2 pos, v2 size )
  {
    ImGuiRect result;
    result.mMini = pos;
    result.mMaxi = pos + size;
    return result;
  }

  ImGuiRect ImGuiRect::FromMinMax( v2 mini, v2 maxi )
  {
    ImGuiRect result;
    result.mMini = mini;
    result.mMaxi = maxi;
    return result;
  }

  float     ImGuiRect::GetWidth() const
  {
    return mMaxi.x - mMini.x;
  }

  float     ImGuiRect::GetHeight() const
  {
    return mMaxi.y - mMini.y;
  }

  bool      ImGuiRect::ContainsPoint( v2 p ) const
  {
    return
      p.x > mMini.x &&
      p.x < mMaxi.x &&
      p.y > mMini.y &&
      p.y < mMaxi.y;
  }

  v2        ImGuiRect::GetDimensions() const
  {
    return mMaxi - mMini;
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
  void ImGuiSetNextWindowSize( v2 size )
  {
    gNextWindow.mSize = size;
  }

  // [ ] Q: imgui.begin should always be followed by a imgui.end,
  //        regardless of the imgui.begin return value.
  //        why is that?
  bool ImGuiBegin( const StringView& name )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.FindWindow( name );
    if( window )
    {
    }
    else
    {
      DesktopWindowHandle desktopWindowHandle = gNextWindow.mDesktopWindowHandle;
      bool owned;
      int desktopWindowWidth;
      int desktopWindowHeight;
      if( desktopWindowHandle.IsValid() )
      {
        const DesktopWindowState* desktopWindowState = GetDesktopWindowState( desktopWindowHandle );
        TAC_ASSERT( desktopWindowState );
        TAC_ASSERT( desktopWindowState->mNativeWindowHandle );
        desktopWindowWidth = desktopWindowState->mWidth;
        desktopWindowHeight = desktopWindowState->mHeight;
        owned = false;
      }
      else
      {
        // vvv --- begin --- move to fn
        Json* windowsJson = SettingsGetJson( "imgui.windows" );
        Json* windowJson = nullptr;
        for( Json* curWindowJson : windowsJson->mArrayElements )
          if( SettingsGetString( "name", "", curWindowJson ) == name )
            windowJson = curWindowJson;

        int x = 50;
        int y = 50;
        int w = gNextWindow.mSize.x ? ( int )gNextWindow.mSize.x : 800;
        int h = gNextWindow.mSize.y ? ( int )gNextWindow.mSize.y : 600;

        if( windowJson )
        {
          x = ( int )SettingsGetNumber( "x", x, windowJson );
          y = ( int )SettingsGetNumber( "y", y, windowJson );
          w = ( int )SettingsGetNumber( "w", w, windowJson );
          h = ( int )SettingsGetNumber( "h", h, windowJson );
        }
        else
        {
          windowJson = windowsJson->AddChild();
          SettingsSetString( "name", name, windowJson );
          SettingsSetNumber( "x", x, windowJson );
          SettingsSetNumber( "y", y, windowJson );
          SettingsSetNumber( "w", w, windowJson );
          SettingsSetNumber( "h", h, windowJson );
        }

        // ^^^ --- begin --- move to fn
        desktopWindowHandle = DesktopAppCreateWindow( x, y, w, h );
        desktopWindowWidth = w;
        desktopWindowHeight = h;
      }

      v2 size = gNextWindow.mSize;
      size.x += size.x > 0 ? 0 : desktopWindowWidth;
      size.y += size.y > 0 ? 0 : desktopWindowHeight;

      TAC_ASSERT( desktopWindowHandle.IsValid() );
      window = TAC_NEW ImGuiWindow;
      window->mName = name;
      window->mDrawData = TAC_NEW UI2DDrawData;
      window->mDesktopWindowHandle = desktopWindowHandle;
      window->mDesktopWindowHandleOwned = !gNextWindow.mDesktopWindowHandle.IsValid();
      window->mPosViewport = {};
      window->mSize = size;
      //window->mDesktopWindowOffset = {};
      //window->mDesktopWindowOffsetExists = true;
      window->mStretchWindow = gNextWindow.mStretch; // !gNextWindow.mDesktopWindowHandle.IsValid() && gNextWindow.mSize == v2( 0, 0 );
      // window->mStretchWindow = !gNextWindow.mDesktopWindowHandle.IsValid() && gNextWindow.mSize == v2( 0, 0 );
      ImGuiGlobals::Instance.mAllWindows.push_back( window );

    }

    gNextWindow = ImGuiNextWindow();
    //gNextWindow.Clear();
    TAC_ASSERT( window->mSize.x > 0 && window->mSize.y > 0 );

    // todo: move this to a ImGuiGlobals::Instance.mFrameData
    TAC_ASSERT( ImGuiGlobals::Instance.mWindowStack.empty() );
    ImGuiGlobals::Instance.mWindowStack.push_back( window );
    ImGuiGlobals::Instance.mCurrentWindow = window;
    window->BeginFrame();

    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( window->mDesktopWindowHandle );
    if( desktopWindowState && window->mStretchWindow )
      window->mSize = v2( ( float )desktopWindowState->mWidth, ( float )desktopWindowState->mHeight );
    return desktopWindowState->mNativeWindowHandle;
  }

  void ImGuiEnd()
  {
    ImGuiGlobals::Instance.mWindowStack.pop_back();
    ImGuiGlobals::Instance.mCurrentWindow =
      ImGuiGlobals::Instance.mWindowStack.empty() ? nullptr :
      ImGuiGlobals::Instance.mWindowStack.back();
  }


  bool ImGuiIsMouseHoveringRectScreenspace( ImGuiRect rectScreenspace )
  {
    const v2 screenspaceMousePos = gKeyboardInput.mCurr.mScreenspaceCursorPos;
    return rectScreenspace.ContainsPoint( screenspaceMousePos );
  }

  void ImGuiBeginChild( const StringView& name, v2 size )
  {
    ImGuiWindow* child = ImGuiGlobals::Instance.FindWindow( name );
    ImGuiWindow* parent = ImGuiGlobals::Instance.mCurrentWindow;
    if( !child )
    {
      child = TAC_NEW ImGuiWindow;
      child->mName = name;
      child->mParent = parent;
      child->mDrawData = parent->mDrawData;
      ImGuiGlobals::Instance.mAllWindows.push_back( child );
    }
    child->mSize = {
      size.x > 0 ? size.x : size.x + parent->mSize.x,
      size.y > 0 ? size.y : size.y + parent->mSize.y };
    ImGuiGlobals::Instance.mWindowStack.push_back( child );
    ImGuiGlobals::Instance.mCurrentWindow = child;
    child->BeginFrame();
  }

  void ImGuiEndChild()
  {
    ImGuiWindow* child = ImGuiGlobals::Instance.mCurrentWindow;
    child->mParent->ItemSize( child->mSize );
    ImGuiGlobals::Instance.mWindowStack.pop_back();
    ImGuiGlobals::Instance.mCurrentWindow = ImGuiGlobals::Instance.mWindowStack.back();
  }

  void ImGuiBeginGroup()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    GroupData groupData = {};
    groupData.mSavedCursorDrawPos = window->mCurrCursorViewport;
    groupData.mSavedLineHeight = window->mCurrLineHeight;

    window->mXOffsets.push_back( window->mCurrCursorViewport.x - window->mPosViewport.x );
    window->mCurrLineHeight = 0;

    //window->mMaxiCursorDrawPos = window->mCurrCursorDrawPos;

    window->mGroupSK.push_back( groupData );
  }

  void ImGuiEndGroup()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    GroupData& groupData = window->mGroupSK.back();
    window->mXOffsets.pop_back();
    //v2 groupEndPos = {
    //  window->mMaxiCursorDrawPos.x,
    //  window->mMaxiCursorDrawPos.y + window->mPrevLineHeight };
    //v2 groupSize = groupEndPos - groupData.mSavedCursorDrawPos;
    v2 groupSize = window->mMaxiCursorViewport - groupData.mSavedCursorDrawPos;
    //groupSize.y = Max( groupSize.y, window->mCurrLineHeight );

    window->mCurrLineHeight = groupData.mSavedLineHeight;

    window->mCurrCursorViewport = groupData.mSavedCursorDrawPos;
    window->ItemSize( groupSize );
    window->mGroupSK.pop_back();
  }

  void ImGuiIndent()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    window->mCurrCursorViewport.x += 15.0f;
    window->mXOffsets.push_back( window->mCurrCursorViewport.x - window->mPosViewport.x );
  }

  void ImGuiUnindent()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    window->mXOffsets.pop_back();
    window->mCurrCursorViewport.x = window->mPosViewport.x + window->mXOffsets.back();
  }

  void ImGuiSameLine()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    window->mCurrCursorViewport = {
      window->mPrevCursorViewport.x + ImGuiGlobals::Instance.mUIStyle.itemSpacing.x,
      window->mPrevCursorViewport.y };
    window->mCurrLineHeight = window->mPrevLineHeight;
    //window->mCurrLineHeight = window->mPrevLineHeight;
  }

  void ImGuiText( const StringView& utf8 )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData;
    v2 textPos = window->mCurrCursorViewport;
    v2 textSize = CalculateTextSize( utf8, ImGuiGlobals::Instance.mUIStyle.fontSize );
    window->ItemSize( textSize );
    bool clipped;
    auto clipRect = ImGuiRect::FromPosSize( textPos, textSize );
    window->ComputeClipInfo( &clipped, &clipRect );
    if( clipped )
      return;
    drawData->AddText( textPos, ImGuiGlobals::Instance.mUIStyle.fontSize, utf8, ImGuiGlobals::Instance.mUIStyle.textColor, &clipRect );
  }

  bool ImGuiInputText( const StringView& label, String& text )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    TextInputData* inputData = window->mTextInputData;
    UI2DDrawData* drawData = window->mDrawData;// ImGuiGlobals::Instance.mUI2DDrawData;
    ImGuiId id = window->GetID();

    bool textChanged = false;

    v2 pos = window->mCurrCursorViewport;

    // Word wrap?
    int lineCount = 1;
    for( char c : text )
      if( c == '\n' )
        lineCount++;

    v2 totalSize = {
      window->mContentRect.mMaxi.x - pos.x,
      ( float )lineCount * ( float )ImGuiGlobals::Instance.mUIStyle.fontSize };

    window->ItemSize( totalSize );

    bool clipped;
    auto clipRect = ImGuiRect::FromPosSize( pos, totalSize );
    window->ComputeClipInfo( &clipped, &clipRect );
    if( clipped )
      return textChanged;

    if( window->IsHovered( clipRect ) && gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
      window->SetActiveID( id );

    v3 textBackgroundColor3 = { 1, 1, 0 };

    v4 editTextColor = { 0, 0, 0, 1 };
    v2 textBackgroundMaxi = {
      pos.x + totalSize.x * ( 2.0f / 3.0f ),
      pos.y + totalSize.y };
    v2 textPos = {
      pos.x + ImGuiGlobals::Instance.mUIStyle.buttonPadding,
      pos.y };
    Render::TextureHandle texture;
    drawData->AddBox( pos,
                      textBackgroundMaxi,
                      v4( textBackgroundColor3, 1 ),
                      texture,
                      &clipRect );

    if( window->GetActiveID() == id )
    {
      CodepointView oldCodepoints = UTF8ToCodepoints( text );
      CodepointView newCodepoints = CodepointView( inputData->mCodepoints.data(),
                                                   inputData->mCodepoints.size() );

      if( oldCodepoints != newCodepoints )
      {
        inputData->SetCodepoints( newCodepoints );
        inputData->mCaretCount = 0;
      }
      TextInputDataUpdateKeys( inputData, textPos );

      // handle double click
      static double lastMouseReleaseSeconds;
      static v2 lastMousePositionDesktopWindowspace;
      if( gKeyboardInput.HasKeyJustBeenReleased( Key::MouseLeft ) &&
          window->IsHovered( clipRect ) &&
          !inputData->mCodepoints.empty() &&
          gKeyboardInput.mCurr.mScreenspaceCursorPosErrors.empty() )
      {
        const v2 screenspaceMousePos = gKeyboardInput.mCurr.mScreenspaceCursorPos;
        auto mouseReleaseSeconds = ImGuiGlobals::Instance.mElapsedSeconds;
        if( mouseReleaseSeconds - lastMouseReleaseSeconds < 0.5f &&
            lastMousePositionDesktopWindowspace == screenspaceMousePos )
        {
          inputData->mNumGlyphsBeforeCaret[ 0 ] = 0;
          inputData->mNumGlyphsBeforeCaret[ 1 ] = inputData->mCodepoints.size();
          inputData->mCaretCount = 2;
        }
        lastMouseReleaseSeconds = mouseReleaseSeconds;
        lastMousePositionDesktopWindowspace = screenspaceMousePos;
      }

      TextInputDataDrawSelection( inputData, drawData, textPos, &clipRect );


      StringView newText = CodepointsToUTF8( newCodepoints );
      if( text != newText )
      {
        text = newText;
        textChanged = true;
      }
    }

    drawData->AddText( textPos,
                       ImGuiGlobals::Instance.mUIStyle.fontSize,
                       text,
                       editTextColor,
                       &clipRect );

    const v2 labelPos = v2( textBackgroundMaxi.x + ImGuiGlobals::Instance.mUIStyle.itemSpacing.x,
                            pos.y );
    drawData->AddText( labelPos,
                       ImGuiGlobals::Instance.mUIStyle.fontSize,
                       label,
                       ImGuiGlobals::Instance.mUIStyle.textColor,
                       &clipRect );

    return textChanged;
  }

  bool ImGuiSelectable( const StringView& str, bool selected )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData;
    bool clicked = false;
    const v2 buttonPosViewport = window->mCurrCursorViewport;
    const v2 buttonSize = {
      window->mContentRect.mMaxi.x - buttonPosViewport.x,
      ( float )ImGuiGlobals::Instance.mUIStyle.fontSize };

    window->ItemSize( buttonSize );
    const ImGuiId id = window->GetID();

    bool clipped;
    ImGuiRect clipRectViewport = ImGuiRect::FromPosSize( buttonPosViewport, buttonSize );
    window->ComputeClipInfo( &clipped, &clipRectViewport );
    if( clipped )
      return clicked;

    v3 color3 = v3( 0.7f, 0.3f, 0.3f ) * 0.7f;
    if( selected )
      color3 = ( color3 + v3( 1, 1, 1 ) ) * 0.3f;

    const bool hovered = window->IsHovered( clipRectViewport );
    if( hovered )
    {
      color3 /= 2.0f;
      if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
      {
        color3 /= 2.0f;
        clicked = true;
        window->SetActiveID( id );
      }
    }

    v4 color( color3, 1 );

    Render::TextureHandle texture;
    drawData->AddBox( buttonPosViewport, buttonPosViewport + buttonSize, color, texture, &clipRectViewport );
    drawData->AddText( buttonPosViewport,
                       ImGuiGlobals::Instance.mUIStyle.fontSize,
                       str,
                       ImGuiGlobals::Instance.mUIStyle.textColor,
                       &clipRectViewport );
    return clicked;
  }

  bool ImGuiButton( const StringView& str )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData;
    const v2 textSize = CalculateTextSize( str, ImGuiGlobals::Instance.mUIStyle.fontSize );
    const v2 buttonSize( textSize.x + 2 * ImGuiGlobals::Instance.mUIStyle.buttonPadding,
                         textSize.y );
    v2 pos = window->mCurrCursorViewport;
    window->ItemSize( textSize );


    bool clipped;
    auto clipRect = ImGuiRect::FromPosSize( pos, buttonSize );
    window->ComputeClipInfo( &clipped, &clipRect );
    if( clipped )
      return false;

    const bool hovered = window->IsHovered( clipRect );

    bool justClicked = false;
    v3 outerBoxColor = v3( .23f, .28f, .38f );
    if( hovered )
    {
      outerBoxColor /= 2.0f;
      if( gKeyboardInput.IsKeyDown( Key::MouseLeft ) )
      {
        outerBoxColor /= 2.0f;
      }
      if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
      {
        justClicked = true;
      }
    }

    Render::TextureHandle texture;
    drawData->AddBox( pos, pos + buttonSize, v4( outerBoxColor, 1 ), texture, &clipRect );

    v2 textPos = {
      pos.x + ImGuiGlobals::Instance.mUIStyle.buttonPadding,
      pos.y };
    drawData->AddText( textPos,
                       ImGuiGlobals::Instance.mUIStyle.fontSize,
                       str,
                       ImGuiGlobals::Instance.mUIStyle.textColor,
                       &clipRect );

    return justClicked;
  }

  bool ImGuiCheckbox( const StringView& str, bool* value )
  {
    const bool oldValue = *value;
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData;
    v2 pos = window->mCurrCursorViewport;

    v2 textSize = CalculateTextSize( str, ImGuiGlobals::Instance.mUIStyle.fontSize );

    float boxWidth = textSize.y;
    v2 boxSize = v2( 1, 1 ) * boxWidth;

    v2 totalSize = v2( boxWidth + ImGuiGlobals::Instance.mUIStyle.itemSpacing.x + textSize.x, textSize.y );
    window->ItemSize( totalSize );

    bool clipped;
    auto clipRect = ImGuiRect::FromPosSize( pos, totalSize );
    window->ComputeClipInfo( &clipped, &clipRect );
    if( clipped )
      return false;

    bool hovered = window->IsHovered( clipRect );

    v4 outerBoxColor = v4( 1, 1, 0, 1 );
    if( hovered )
    {
      outerBoxColor = v4( 0.5f, 0.5f, 0, 1 );
      if( gKeyboardInput.IsKeyDown( Key::MouseLeft ) )
      {
        outerBoxColor = v4( 0.3f, 0.3f, 0, 1 );
      }
      if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
      {
        *value = !*value;

        gKeyboardInput.SetIsKeyDown( Key::MouseLeft, false );
      }
    }

    Render::TextureHandle noTexture;
    drawData->AddBox( pos, pos + boxSize, outerBoxColor, noTexture, &clipRect );
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
        drawData->mDefaultIndex2Ds.push_back( ( UI2DIndex )( iVert + 0 ) );
        drawData->mDefaultIndex2Ds.push_back( ( UI2DIndex )( iVert + 1 ) );
        drawData->mDefaultIndex2Ds.push_back( ( UI2DIndex )( iVert + 2 ) );
        drawData->mDefaultIndex2Ds.push_back( ( UI2DIndex )( iVert + 1 ) );
        drawData->mDefaultIndex2Ds.push_back( ( UI2DIndex )( iVert + 3 ) );
        drawData->mDefaultIndex2Ds.push_back( ( UI2DIndex )( iVert + 2 ) );
        drawData->mDefaultVertex2Ds.resize( iVert + 4 );
        UI2DVertex* defaultVertex2D = &drawData->mDefaultVertex2Ds[ iVert ];
        defaultVertex2D->mPosition = p0;
        defaultVertex2D++;
        defaultVertex2D->mPosition = p1;
        defaultVertex2D++;
        defaultVertex2D->mPosition = p2;
        defaultVertex2D++;
        defaultVertex2D->mPosition = p3;

        DefaultCBufferPerObject perObjectData = {};
        perObjectData.World = m4::Identity();
        perObjectData.Color = checkmarkColor;

        UI2DDrawCall drawCall;
        drawCall.mIndexCount = 6;
        drawCall.mIIndexStart = iIndex;
        drawCall.mVertexCount = 4;
        drawCall.mIVertexStart = iVert;
        drawCall.mShader = gUI2DCommonData.mShader;
        drawCall.mUniformSource = perObjectData;
        drawData->mDrawCall2Ds.push_back( drawCall );
      }
      else
      {
        v2 innerPadding = v2( 1, 1 ) * 3;
        drawData->AddBox( pos + innerPadding,
                          pos + boxSize - innerPadding,
                          checkmarkColor,
                          Render::TextureHandle(),
                          &clipRect );
      }
    }

    v2 textPos = {
      pos.x + boxWidth + ImGuiGlobals::Instance.mUIStyle.itemSpacing.x,
      pos.y };
    drawData->AddText( textPos, ImGuiGlobals::Instance.mUIStyle.fontSize, str, ImGuiGlobals::Instance.mUIStyle.textColor, &clipRect );
    return oldValue != *value;
  }

  void ImGuiSetCursorPos( v2 local )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    window->mCurrCursorViewport = local;
  }

  void ImGuiImage( const int hTex, const v2 size )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    const v2 pos = window->mCurrCursorViewport;
    const v2 boxMini = pos;
    const v2 boxMaxi = pos + size;
    window->ItemSize( size );
    window->mDrawData->AddBox( boxMini, boxMaxi, v4( 1, 1, 1, 1 ), Render::TextureHandle( hTex ), nullptr );
  }

  bool ImGuiDragFloat( const StringView& str, float* value )
  {
    auto getter = []( String& to, const void* from )
    {
      float i = *( ( float* )from );
      to = ToString( i );
    };
    auto setter = []( StringView from, void* to )
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
    const bool result = ImguiDragVal( str, value, sizeof( float ), getter, setter, whatToDoWithMousePixel );
    return result;
  }

  bool ImGuiDragFloat2( const StringView& str, float* value )
  {
    const bool x = ImGuiDragFloat( str + " x", value + 0 );
    const bool y = ImGuiDragFloat( str + " y", value + 1 );
    return x || y;
  }
  bool ImGuiDragFloat3( const StringView& str, float* value )
  {
    const bool x = ImGuiDragFloat( str + " x", value + 0 );
    const bool y = ImGuiDragFloat( str + " y", value + 1 );
    const bool z = ImGuiDragFloat( str + " z", value + 2 );
    return x || y || z;
  }
  bool ImGuiDragFloat4( const StringView& str, float* value )
  {
    const bool x = ImGuiDragFloat( str + " x", value + 0 );
    const bool y = ImGuiDragFloat( str + " y", value + 1 );
    const bool z = ImGuiDragFloat( str + " z", value + 2 );
    const bool w = ImGuiDragFloat( str + " w", value + 3 );
    return x || y || z || w;
  }

  bool ImGuiDragInt( const StringView& str, int* value )
  {
    auto getter = []( String& to, const void* from )
    {
      int i = *( ( int* )from );
      to = ToString( i );
    };
    auto setter = []( StringView from, void* to )
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
    const bool result = ImguiDragVal( str, value, sizeof( int ), getter, setter, whatToDoWithMousePixel );
    return result;
  }

  bool ImGuiCollapsingHeader( const StringView& name )
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData; // ImGuiGlobals::Instance.mUI2DDrawData;
    v2 pos = window->mCurrCursorViewport;
    v2 totalSize = {
      window->mContentRect.mMaxi.x - pos.x,
      ( float )ImGuiGlobals::Instance.mUIStyle.fontSize };
    window->ItemSize( totalSize );
    bool clipped;
    auto clipRect = ImGuiRect::FromPosSize( pos, totalSize );
    window->ComputeClipInfo( &clipped, &clipRect );
    if( clipped )
      return false;

    v4 backgroundBoxColor = { 100 / 255.0f, 65 / 255.0f, 164 / 255.0f, 255 / 255.0f };
    ImGuiId id = window->GetID();
    bool& isOpen = window->mCollapsingHeaderStates[ id ];
    if( window->IsHovered( clipRect ) )
    {
      backgroundBoxColor.xyz() /= 2.0f;
      if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
        isOpen = !isOpen;

      //const bool down = gKeyboardInput.IsKeyDown( Key::MouseLeft );
      //std::cout << "down: " << ( down? "true" : "false" ) << std::endl;
    }

    drawData->AddBox( pos,
                      pos + totalSize,
                      backgroundBoxColor,
                      Render::TextureHandle(),
                      &clipRect );

    v4 textColor = { 1, 1, 1, 1 };
    v2 textPos = { pos.x + ImGuiGlobals::Instance.mUIStyle.buttonPadding, pos.y };
    drawData->AddText( textPos, ImGuiGlobals::Instance.mUIStyle.fontSize, isOpen ? "v" : ">", textColor, &clipRect );
    textPos.x += CalculateTextSize( "  ", ImGuiGlobals::Instance.mUIStyle.fontSize ).x;
    drawData->AddText( textPos, ImGuiGlobals::Instance.mUIStyle.fontSize, name, textColor, &clipRect );

    return isOpen;
  }

  void ImGuiPushFontSize( int value )
  {
    ImGuiGlobals::Instance.mFontSizeSK.push_back( ImGuiGlobals::Instance.mUIStyle.fontSize );
    ImGuiGlobals::Instance.mUIStyle.fontSize = value;
  }

  void ImGuiPopFontSize()
  {
    int fontsize = ImGuiGlobals::Instance.mFontSizeSK.back();
    ImGuiGlobals::Instance.mUIStyle.fontSize = fontsize;
    ImGuiGlobals::Instance.mFontSizeSK.pop_back();
  }

  void ImGuiBeginMenuBar()
  {
    ImGuiWindow* window = ImGuiGlobals::Instance.mCurrentWindow;
    UI2DDrawData* drawData = window->mDrawData;// ImGuiGlobals::Instance.mUI2DDrawData;
    TAC_ASSERT( !window->mIsAppendingToMenu );
    window->mIsAppendingToMenu = true;
    v2 size = { window->mSize.x, ImGuiGlobals::Instance.mUIStyle.fontSize + ImGuiGlobals::Instance.mUIStyle.buttonPadding * 2 };
    v4 color = { v3( 69, 45, 83 ) / 255.0f, 1.0f };
    drawData->AddBox( {}, size, color, Render::TextureHandle(), ( ImGuiRect* )nullptr );
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
    String text =
      "Cur window active id: " +
      ToString( ImGuiGlobals::Instance.mCurrentWindow->GetActiveID() );
    ImGuiText( text );
  }


  static void ImGuiRenderWindow( ImGuiWindow* window, Errors& errors )
  {
    // The child draw calls are stored in mParent->mDrawdata
    if( window->mParent )
      return;

    // remember, these are imguiwindows, not win32windows
    const char* groupName = va( "imgui for window %s", window->mName.c_str() );
    TAC_RENDER_GROUP_BLOCK( groupName );

    TAC_ASSERT( window->mDesktopWindowHandle.IsValid() );
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( window->mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    const Render::ViewHandle viewHandle = WindowGraphicsGetView( window->mDesktopWindowHandle );
    if( !viewHandle.IsValid() )
      return;

    if( window->mDesktopWindowHandleOwned )
    {
      const float w = ( float )desktopWindowState->mWidth;
      const float h = ( float )desktopWindowState->mHeight;
      const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( window->mDesktopWindowHandle );
      Render::SetViewFramebuffer( viewHandle, framebufferHandle );
      Render::SetViewport( viewHandle, Viewport( w, h ) );
      Render::SetViewScissorRect( viewHandle, ScissorRect( w, h ) );
    }
    window->mDrawData->DrawToTexture( viewHandle,
                                      desktopWindowState->mWidth,
                                      desktopWindowState->mHeight,
                                      errors );
  }

  static void ImGuiRender( Errors& errors )
  {
    // this should iteratre through imgui views?
    // and the draw data shouldnt exist, it should just be inlined here
    // in 1 big ass vbo/ibo
    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
      ImGuiRenderWindow( window, errors );
  }

  void ImGuiFrameBegin( double elapsedSeconds, const DesktopWindowHandle& mouseHoveredWindow )
  {
    gKeyboardInput.mCurr.mScreenspaceCursorPos;
    ImGuiGlobals::Instance.mElapsedSeconds = elapsedSeconds;
    ImGuiGlobals::Instance.mMouseHoveredWindow = mouseHoveredWindow;
  }

  static bool ImGuiDesktopWindowOwned( DesktopWindowHandle desktopWindowHandle )
  {
    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
      if( window->mDesktopWindowHandleOwned && window->mDesktopWindowHandle == desktopWindowHandle )
        return true;
    return false;
  }

  void ImGuiFrameEnd( Errors& errors )
  {
    ImGuiRender( errors );

    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
      if( window->mDesktopWindowHandleOwned )
      {
        const DesktopWindowRect rect = GetDesktopWindowRectWindowspace( window->mDesktopWindowHandle );
        DesktopAppResizeControls( window->mDesktopWindowHandle, 7 );
        DesktopAppMoveControls( window->mDesktopWindowHandle, rect );
      }
  }

  void ImGuiInit()
  {

  }

  static void ImGuiSaveWindowSettings( ImGuiWindow* window )
  {
    Json* windowsJson = SettingsGetJson( "imgui.windows" );
    Json* windowJson = nullptr;
    for( Json* curWindowJson : windowsJson->mArrayElements )
      if( SettingsGetString( "name", "", curWindowJson ) == window->mName )
        windowJson = curWindowJson;

    windowJson = windowJson ? windowJson : windowsJson->AddChild();
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( window->mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;
    SettingsSetString( "name", window->mName, windowJson );
    SettingsSetNumber( "x", desktopWindowState->mX, windowJson );
    SettingsSetNumber( "y", desktopWindowState->mY, windowJson );
    SettingsSetNumber( "w", desktopWindowState->mWidth, windowJson );
    SettingsSetNumber( "h", desktopWindowState->mHeight, windowJson );
  }

  void ImGuiSaveWindowSettings()
  {
    for( ImGuiWindow* window : ImGuiGlobals::Instance.mAllWindows )
      if( window->mDesktopWindowHandleOwned )
        ImGuiSaveWindowSettings( window );
  }

  void ImGuiUninit()
  {
    ImGuiSaveWindowSettings();
  }
}
