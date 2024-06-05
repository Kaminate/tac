#include "tac_imgui_drag.h"
#include "tac_imgui_state.h"

#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-engine-core/graphics/ui/tac_text_edit.h"

namespace Tac
{

  enum class DragMode
  {
    Drag,
    TextInput
  };

  struct DragData
  {
    DragMode       mMode             { ( DragMode )0 };
    float          mDragDistPx       { 0 };

    // This is the value of the variable we are gizmoing for at the start of the mouse drag.
    // That way, we can accumulate all the mouse drag pixels and apply them to ints in
    // addition to floats
    char           mValueCopy[ 10 ]  {};
  };

  static const ImGuiRscIdx sDragDataID{ ImGuiRegisterWindowResource( TAC_STRINGIFY( DragData ),
                                                                     nullptr,
                                                                     sizeof( DragData ) ) };


  using Drag_Getter = String( * )( const void* );
  using Drag_Setter = void( * )( const StringView& from, void* to );
  using Drag_MouseHandler = void( * )( float mouseChangeSinceBeginningOfDrag,
                                       const void* valAtDragStart,
                                       void* curVal );

  static bool ImguiDragValNoLabel( float width,
                                   void* valueBytes,
                                   int valueByteCount,
                                   Drag_Getter getter,
                                   Drag_Setter setter,
                                   Drag_MouseHandler mouseHandler )
  {
    ImGuiGlobals& globals { ImGuiGlobals::Instance };
    SimKeyboardApi* keyboardApi { globals.mSimKeyboardApi };
    SimWindowApi* windowApi { globals.mSimWindowApi };

    const float fontSize { ImGuiGetFontSize() };
    const float buttonPadding { ImGuiGetButtonPadding() };

    String valueStr { getter( valueBytes ) };
    bool changed { false };

    ImGuiWindow* window { globals.mCurrentWindow };
    UI2DDrawData* drawData { window->mDrawData };
    TextInputData* inputData { window->mTextInputData };

    const ImGuiID id { window->mIDStack.back() }; // assumption
    const v2 pos { window->mViewportSpaceCurrCursor };
    const v2 totalSize( width, fontSize );
    const ImGuiRect origRect { ImGuiRect::FromPosSize( pos, totalSize ) };

    DragData* dragFloatData { ( DragData* )window->GetWindowResource( sDragDataID, id ) };

    window->ItemSize( totalSize );
    if( !window->Overlaps( origRect ) )
      return false;

    const ImGuiRect clipRect { window->Clip( origRect ) };
    const v2 valuePos { pos + v2( buttonPadding, 0 ) };
    const bool hovered { window->IsHovered( clipRect ) };

    //static Timestamp consumeMouse;
    //if( hovered )
    //  Mouse::TryConsumeMouseMovement( &consumeMouse, TAC_STACK_FRAME );

    if( hovered && keyboardApi->JustPressed( Key::MouseLeft ) )
      SetActiveID( id, window );


    const bool active { GetActiveID() == id };

    if(active)
    {
      if( dragFloatData->mMode == DragMode::Drag )
      {
        v2 screenspaceMousePos { keyboardApi->GetMousePosScreenspace() };
        static float lastMouseXDesktopWindowspace;


        if( keyboardApi->JustPressed( Key::MouseLeft ) )
        {
          lastMouseXDesktopWindowspace = screenspaceMousePos.x;
          dragFloatData->mDragDistPx = 0;
          MemCpy( dragFloatData->mValueCopy, valueBytes, valueByteCount );
        }
        else if ( keyboardApi->IsPressed( Key::MouseLeft ) )
        {
          WindowHandle windowHandle = window->GetWindowHandle();
          const v2 desktopWindowPos = windowApi->GetPos( windowHandle );
          const v2 viewportSpaceMousePos = screenspaceMousePos - desktopWindowPos;

          float moveCursorDir = 0;
          moveCursorDir = viewportSpaceMousePos.x > clipRect.mMaxi.x ? -1 : moveCursorDir;
          moveCursorDir = viewportSpaceMousePos.x < clipRect.mMini.x ? 1 : moveCursorDir;
          if( moveCursorDir )
          {
            Errors errors;
            screenspaceMousePos.x += moveCursorDir * clipRect.GetWidth();
            OS::OSSetScreenspaceCursorPos( screenspaceMousePos, errors );
          }
          else
          {
            dragFloatData->mDragDistPx += screenspaceMousePos.x - lastMouseXDesktopWindowspace;
            mouseHandler( dragFloatData->mDragDistPx, dragFloatData->mValueCopy, valueBytes );
            changed = true;
          }
        }

        lastMouseXDesktopWindowspace = screenspaceMousePos.x;

        // handle double click
        static Timestamp lastMouseReleaseSeconds;
        static v2 lastMousePositionDesktopWindowspace;
        if( keyboardApi->JustDepressed( Key::MouseLeft ) && hovered )
        {
          const Timestamp mouseReleaseSeconds { ImGuiGlobals::Instance.mElapsedSeconds };
          const TimestampDifference kDoubleClickSecs = 0.5f;
          if( mouseReleaseSeconds - lastMouseReleaseSeconds < kDoubleClickSecs &&
              lastMousePositionDesktopWindowspace == screenspaceMousePos )
          {
            const CodepointString codepointString{ UTF8ToCodepointString( valueStr ) };
            const CodepointView codepoints{ codepointString.data(), codepointString.size() };
            inputData->SetCodepoints( codepoints );
            inputData->mCaretCount = 2;
            inputData->mNumGlyphsBeforeCaret[ 0 ] = 0;
            inputData->mNumGlyphsBeforeCaret[ 1 ] = codepoints.size();
            dragFloatData->mMode = DragMode::TextInput;
          }
          lastMouseReleaseSeconds = mouseReleaseSeconds;
          lastMousePositionDesktopWindowspace = screenspaceMousePos;
        }
      }

      if( dragFloatData->mMode == DragMode::TextInput )
      {
        const auto oldCodepoints { inputData->mCodepoints };
        TextInputDataUpdateKeys( inputData, window->GetMousePosViewport(), valuePos );

        const bool codepointsChanged { oldCodepoints != inputData->mCodepoints };
        //const bool codepointsChanged = oldCodepoints.size() != inputData->mCodepoints.size()
        //  || 0 != MemCmp( oldCodepoints.data(),
        //                  inputData->mCodepoints.data(),
        //                  oldCodepoints.size() * sizeof( Codepoint ) );
        if( codepointsChanged )
        {
          changed = true;

          const StringView newText { CodepointsToUTF8(inputData->GetCodepointView()) };
          setter( newText, valueBytes );
          valueStr = newText;

          // tab between x,y,z for imguidragfloat3
          //if( keyboardApi->JustPressed( Key::Tab ) )
          //  window->mIDAllocator->mActiveID++;
        }
      }
    }

    if( id != GetActiveID() )
    {
      if( dragFloatData->mMode == DragMode::TextInput )
      {
        dragFloatData->mMode = DragMode::Drag;
        dragFloatData->mDragDistPx = 0;
      }
      else
      {
        auto remove_trailing_zeroes = []( String& s )
        {
          if( !s.contains( '.' ) )
            return;
          while( s.back() == '0' )
            s.pop_back();
          if( s.back() == '.' )
            s.pop_back();
        };
        remove_trailing_zeroes( valueStr );
      }
    }

    ImGuiCol colEnum { ImGuiCol::FrameBG };
    if( hovered )
      colEnum = ImGuiCol::FrameBGHovered;
    if( active )
      colEnum = ImGuiCol::FrameBGActive;
      
    const v4& color { ImGuiGetColor( colEnum ) };

    const UI2DDrawData::Box box
    {
      .mMini  { pos },
      .mMaxi  { pos + clipRect.GetSize() },
      .mColor { color },
    };
    drawData->AddBox( box, &clipRect );

    if( dragFloatData->mMode == DragMode::TextInput )
      TextInputDataDrawSelection( inputData, drawData, valuePos, &clipRect );

    const UI2DDrawData::Text text
    {
      .mPos      { valuePos },
      .mFontSize { fontSize },
      .mUtf8     { valueStr },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    };
    drawData->AddText( text, &clipRect );

    return changed;
  }

  static bool ImGuiDragTypeN( const StringView& str,
                              int sizeOfT,
                              void* values,
                              int n,
                              Drag_Getter getter,
                              Drag_Setter setter,
                              Drag_MouseHandler mouseHandler )
  {
    ImGuiWindow* window { ImGuiGlobals::Instance.mCurrentWindow };
    const float xMax { window->mViewportSpaceVisibleRegion.mMaxi.x };
    const float xCur { window->mViewportSpaceCurrCursor.x };

    static const float dragNumbersPercentage { 0.66f };

    float dragW { xMax - xCur }; // total remaining usable width
    dragW *= dragNumbersPercentage; // width usable for the drag widgets (not the label)
    dragW -= ( n - 1 ) * ImGuiGlobals::Instance.mUIStyle.itemSpacing.x; // account for sameline
    dragW /= n; // width usable for a single drag widget


    UI2DDrawData* drawData { window->mDrawData };
    drawData->PushDebugGroup( "ImGuiDragTypeN", str );

    PushID( str );

    bool changed { false };
    for( int i{}; i < n; ++i )
    {
      TAC_ASSERT( n < 3 );
      const char* ids[]{ "x", "y", "z" };
      PushID( ids[ i ] );

      void* value { ( char* )values + ( std::ptrdiff_t )( i * sizeOfT ) };
      changed |= ImguiDragValNoLabel( dragW, value, sizeOfT, getter, setter, mouseHandler );
      ImGuiSameLine();

      PopID();
    }

    PopID();

    ImGuiText( str );
    drawData->PopDebugGroup();
    return changed;
  }

  // -----------------------------------------------------------------------------------------------

  static String DragFloatGetter( const void* from )
  {
    const float f { *( float* )from };

    // TODO: replace with return FormatString( "{:.2}", f );
    //       once visual studio doesnt fuck the syntax highlighting
    String s { ToString( f ) };
    const int iDot { s.find( '.' ) };
    if( iDot != s.npos )
    {
      const int numDigitsAfterDecimal { 2 };
      s.resize( Min( s.size(), iDot + numDigitsAfterDecimal + 1 ) );
    }
    return s;
  };

  static void DragFloatSetter( const StringView& from, void* to )
  {
    *( float* )to = ( float )Atof( from.c_str() );
  };

  static void DragFloatMouseHandler( float mouseChangeSinceBeginningOfDrag,
                                     const void* valAtDragStart,
                                     void* curVal )
  {
    const float& valAtDragStartRef { *( const float* )valAtDragStart };
    float& curValRef { *( float* )curVal };
    const float offset { ( float )( mouseChangeSinceBeginningOfDrag * 0.1f ) };
    curValRef = valAtDragStartRef + offset;
  };

  // -----------------------------------------------------------------------------------------------

  static String DragIntGetter(  const void* from ) { return ToString( *( int* )from ); };

  static void DragIntSetter( const StringView& from, void* to )
  {
    *( int* )to = Atoi( from.c_str() );
  };

  static void DragIntMouseHandler ( float mouseChangeSinceBeginningOfDrag,
                                       const void* valAtDragStart,
                                       void* curVal )
  {
    const int& valAtDragStartRef { *( const int* )valAtDragStart };
    int& curValRef { *( int* )curVal };
    const int offset { ( int )( mouseChangeSinceBeginningOfDrag / 50.0f ) };
    curValRef = valAtDragStartRef + offset;
  };



}

  bool Tac::ImGuiDragFloatN( const StringView& str, float* values, int n )
  {
    return ImGuiDragTypeN( str,
                           sizeof( float ),
                           values,
                           n,
                           DragFloatGetter,
                           DragFloatSetter,
                           DragFloatMouseHandler );
  }

  bool Tac::ImGuiDragIntN( const StringView& str, int* values, int n )
  {
    return ImGuiDragTypeN( str,
                           sizeof( int ),
                           values,
                           n,
                           DragIntGetter,
                           DragIntSetter,
                           DragIntMouseHandler );
  }

