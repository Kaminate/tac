#include "tac_imgui_drag.h" // self-inc

#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/graphics/ui/tac_text_edit.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"

namespace Tac
{

  enum class DragMode
  {
    Drag = 0,
    TextInput
  };

  struct DragData
  {
    DragMode       mMode             {};
    float          mDragDistPx       {};

    // This is the value of the variable we are gizmoing for at the start of the mouse drag.
    // That way, we can accumulate all the mouse drag pixels and apply them to ints in
    // addition to floats
    char           mValueCopy[ 10 ]  {};
  };

  static const ImGuiRscIdx sDragDataID{
    ImGuiWindowResource::Register(
      ImGuiWindowResource::Params
      {
        .mName                 { TAC_TYPESAFE_STRINGIFY_TYPE( DragData ) },
        .mInitialDataBytes     { nullptr},
        .mInitialDataByteCount { sizeof( DragData )},
      } ) };


  using Drag_Getter = String( * )( const void* );
  using Drag_Setter = void( * )( const StringView from, void* to );
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

    const float fontSize { ImGuiGetFontSizePx() };
    const float buttonPaddingPx { ImGuiGetButtonPaddingPx() };

    String valueStr { getter( valueBytes ) };
    bool changed {};

    ImGuiWindow* window { ImGuiGlobals::mCurrentWindow };
    UI2DDrawData* drawData { window->mDrawData };
    TextInputData* inputData { window->mTextInputData };
    TAC_ASSERT( inputData );

    const ImGuiID id { window->mIDStack.back() }; // assumption
    const v2 pos { window->mViewportSpaceCurrCursor };
    const v2 totalSize( width, fontSize );
    const ImGuiRect origRect { ImGuiRect::FromPosSize( pos, totalSize ) };

    window->ItemSize( totalSize );
    if( !window->Overlaps( origRect ) )
      return false;

    const ImGuiRect clipRect { window->Clip( origRect ) };
    const v2 valuePos { pos + v2( buttonPaddingPx, 0 ) };
    const bool hovered { window->IsHovered( clipRect, id ) };
    const bool active{ ImGuiGlobals::GetActiveID() == id };

    static float prevMouseX_uiwindowspace;
    DragData* dragFloatData { ( DragData* )window->GetWindowResource( sDragDataID, id ) };

    // Begin drag behavior
    if( hovered
        && !active
        && UIKeyboardApi::JustPressed( Key::MouseLeft ) )
    {
      dragFloatData->mMode = DragMode::Drag;
      ImGuiGlobals::SetActiveID( id, window );
      prevMouseX_uiwindowspace = window->GetMousePos_uiwindowspace().x;
      dragFloatData->mDragDistPx = 0;
      MemCpy( dragFloatData->mValueCopy, valueBytes, valueByteCount );
    }

    // End drag behavior
    if( active
        && dragFloatData->mMode == DragMode::Drag
        && !UIKeyboardApi::IsPressed( Key::MouseLeft ) )
      ImGuiGlobals::ClearActiveID();

    // Continue drag behavior
    if( active
        && dragFloatData->mMode == DragMode::Drag
        && UIKeyboardApi::IsPressed( Key::MouseLeft ) )
    {
      const float dx{ window->GetMousePos_uiwindowspace().x - prevMouseX_uiwindowspace };
      dragFloatData->mDragDistPx += dx;
      mouseHandler( dragFloatData->mDragDistPx, dragFloatData->mValueCopy, valueBytes );

      v2 nextMousePos_uiwindowspace{ window->GetMousePos_uiwindowspace() };
      if( dx )
      {
        changed = true;

        while( nextMousePos_uiwindowspace.x > window->mSize.x )
          nextMousePos_uiwindowspace.x -= window->mSize.x;
        while( nextMousePos_uiwindowspace.x < 0 )
          nextMousePos_uiwindowspace.x += window->mSize.x;

        if( nextMousePos_uiwindowspace.x != window->GetMousePos_uiwindowspace().x )
        {
          v2 nextMousePos_desktopspace = nextMousePos_uiwindowspace + window->GetWindowPos_desktopspace();

          Errors errors;
          OS::OSSetScreenspaceCursorPos( nextMousePos_desktopspace, errors );
          TAC_ASSERT( !errors );

          //prevMousePos_uiwindowspace.x = nextMousePos_uiwindowspace.x;
        }
      }

      prevMouseX_uiwindowspace = nextMousePos_uiwindowspace.x;
    }

    // Begin TextInput behavior
    if( hovered
        && !active
        && UIKeyboardApi::JustPressed( Key::MouseLeft ) )
    {
        static GameTime singleClickTime;
        static v2i singleClickPos;

        if( ( ImGuiGlobals::mElapsedSeconds - singleClickTime ) < GameTimeDelta{ .mSeconds{ 0.5f } } &&
            UIKeyboardApi::GetMousePosScreenspace() == singleClickPos )
        {
          const CodepointString codepointString{ UTF8ToCodepointString( valueStr ) };
          const CodepointView codepoints{ codepointString.data(), codepointString.size() };
          inputData->SetCodepoints( codepoints );
          inputData->mCaretCount = 2;
          inputData->mNumGlyphsBeforeCaret[ 0 ] = 0;
          inputData->mNumGlyphsBeforeCaret[ 1 ] = codepoints.size();
          dragFloatData->mMode = DragMode::TextInput;
          ImGuiGlobals::SetActiveID( id, window );
        }

        singleClickTime = ImGuiGlobals::mElapsedSeconds;
        singleClickPos = UIKeyboardApi::GetMousePosScreenspace();
    }

    // End TextInput behavior
    if( active && dragFloatData->mMode == DragMode::TextInput )
    {
      if( bool clickedAway{ !hovered && UIKeyboardApi::JustPressed( Key::MouseLeft ) };
          clickedAway || UIKeyboardApi::JustPressed( Key::Enter ) )
      {
        ImGuiGlobals::ClearActiveID();
      }
    }

    // Continue TextInput behavior
    if( active && dragFloatData->mMode == DragMode::TextInput )
    {
        const CodepointString oldCodepoints { inputData->mCodepoints };
        TextInputDataUpdateKeys( inputData, window->GetMousePos_uiwindowspace(), valuePos );
        if( oldCodepoints != inputData->mCodepoints )
        {
          const StringView newText { CodepointsToUTF8(inputData->GetCodepointView()) };
          setter( newText, valueBytes );
          valueStr = newText;
          changed = true;
        }
    }


    ImGuiCol colEnum { ImGuiCol::FrameBG };
    if( hovered ) { colEnum = ImGuiCol::FrameBGHovered; }
    if( active ) { colEnum = ImGuiCol::FrameBGActive; }
      
    const v4& color { ImGuiGetColor( colEnum ) };
    drawData->AddBox(
      UI2DDrawData::Box
      {
        .mMini  { pos },
        .mMaxi  { pos + clipRect.GetSize() },
        .mColor { color },
      }, &clipRect );

    if( active && dragFloatData->mMode == DragMode::TextInput )
      TextInputDataDrawSelection( inputData, drawData, valuePos, &clipRect );

    drawData->AddText( UI2DDrawData::Text
    {
      .mPos      { valuePos },
      .mFontSize { fontSize },
      .mUtf8     { valueStr },
      .mColor    { ImGuiGetColor( ImGuiCol::Text ) },
    }, &clipRect );
    return changed;
  }

  static bool ImGuiDragTypeN( const StringView str,
                              int sizeOfT,
                              void* values,
                              int n,
                              Drag_Getter getter,
                              Drag_Setter setter,
                              Drag_MouseHandler mouseHandler )
  {
    ImGuiWindow* window { ImGuiGlobals::mCurrentWindow };
    const float xMax { window->mViewportSpaceVisibleRegion.mMaxi.x };
    const float xCur { window->mViewportSpaceCurrCursor.x };

    static const float dragNumbersPercentage { 0.66f };

    float dragW { xMax - xCur }; // total remaining usable width
    dragW *= dragNumbersPercentage; // width usable for the drag widgets (not the label)
    dragW -= ( n - 1 ) * ImGuiGetItemSpacingPx().x; // account for sameline
    dragW /= n; // width usable for a single drag widget


    UI2DDrawData* drawData { window->mDrawData };
    drawData->PushDebugGroup( ShortFixedString::Concat( "ImGuiDragTypeN(", str, ")" ) );

    PushID( str );

    TAC_ASSERT( n <= 4 );
    const char* ids[]{ "x", "y", "z", "w" };

    bool changed {};
    for( int i{}; i < n; ++i )
    {
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
    String s{ ToString( f ) };
    if( const int iDot{ s.find( '.' ) }; iDot != s.npos )
    {
      const int numDigitsAfterDecimal { 2 };
      s.resize( Min( s.size(), iDot + numDigitsAfterDecimal + 1 ) );
    }
    return s;
  };

  static void DragFloatSetter( const StringView from, void* to )
  {
    *( float* )to = ( float )Atof( from.c_str() );
  };

  static void DragFloatMouseHandler( float mouseChangeSinceBeginningOfDrag,
                                     const void* pValAtDragStart,
                                     void* curVal )
  {
    const float valAtDragStart { *( const float* )pValAtDragStart };
    const float offset { ( float )( mouseChangeSinceBeginningOfDrag * 0.01f ) };
    float& curValRef { *( float* )curVal };
    curValRef = valAtDragStart + offset;
  };

  // -----------------------------------------------------------------------------------------------

  static String DragIntGetter(  const void* from ) { return ToString( *( int* )from ); };

  static void DragIntSetter( const StringView from, void* to )
  {
    *( int* )to = Atoi( from.c_str() );
  };

  static void DragIntMouseHandler ( float mouseChangeSinceBeginningOfDrag,
                                       const void* pValAtDragStart,
                                       void* curVal )
  {
    const int valAtDragStart { *( const int* )pValAtDragStart };
    const int offset { ( int )( mouseChangeSinceBeginningOfDrag / 50.0f ) };
    int& curValRef { *( int* )curVal };
    curValRef = valAtDragStart + offset;
  };
} // namespace Tac

bool Tac::ImGuiDragFloatN( const StringView str, float* values, int n )
{
  return ImGuiDragTypeN( str,
                         sizeof( float ),
                         values,
                         n,
                         DragFloatGetter,
                         DragFloatSetter,
                         DragFloatMouseHandler );
}

bool Tac::ImGuiDragIntN( const StringView str, int* values, int n )
{
  return ImGuiDragTypeN( str,
                         sizeof( int ),
                         values,
                         n,
                         DragIntGetter,
                         DragIntSetter,
                         DragIntMouseHandler );
}

