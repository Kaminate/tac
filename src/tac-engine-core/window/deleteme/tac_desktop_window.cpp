#include "tac_desktop_window.h" // self-inc

#include "tac-std-lib/containers/tac_array.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
//#include "tac-engine-core/shell/tac_shell.h"
//#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static DesktopWindowState sDesktopWindowStates[ kDesktopWindowCapacity ];
  static DesktopWindowHandle sMouseHoveredWindowHandle;

  // -----------------------------------------------------------------------------------------------

  v2   DesktopWindowState::GetPosV2() const  { return { ( float )mX, ( float )mY }; }
  v2   DesktopWindowState::GetSizeV2() const { return { ( float )mWidth, ( float )mHeight }; }

  // -----------------------------------------------------------------------------------------------

  int  DesktopWindowRect::GetArea() const    { return GetWidth() * GetHeight(); }
  bool DesktopWindowRect::IsEmpty() const    { return GetArea() == 0; }
  int  DesktopWindowRect::GetWidth() const   { return mRight - mLeft; }
  int  DesktopWindowRect::GetHeight() const  { return mBottom - mTop; }


  // -----------------------------------------------------------------------------------------------

  DesktopWindowHandle::DesktopWindowHandle( int index ) : mIndex{ index } {}

  bool                DesktopWindowHandle::IsValid() const { return mIndex != -1; }

  int                 DesktopWindowHandle::GetIndex() const { return mIndex; }

  DesktopWindowState* DesktopWindowHandle::GetDesktopWindowState() const
  {
    return IsValid()
      ? &sDesktopWindowStates[ GetIndex() ]
      : nullptr;
  }

  const void*         DesktopWindowHandle::GetDesktopWindowNativeHandle() const
  {
    const DesktopWindowState* state = GetDesktopWindowState();
    return state ? state->mNativeWindowHandle : nullptr;
  }

  DesktopWindowRect   DesktopWindowHandle::GetDesktopWindowRectScreenspace() const
  {
 }    const DesktopWindowState* desktopWindowState = GetDesktopWindowState();
    const DesktopWindowRect desktopWindowRect
    {
      .mLeft   { desktopWindowState->mX },
      .mRight  { desktopWindowState->mX + desktopWindowState->mWidth },
      .mBottom { desktopWindowState->mY + desktopWindowState->mHeight },
      .mTop    { desktopWindowState->mY },
    };
    return desktopWindowRect;
  }

  DesktopWindowRect   DesktopWindowHandle::GetDesktopWindowRectWindowspace() const
  {
    const DesktopWindowState* desktopWindowState { GetDesktopWindowState() };
    const DesktopWindowRect desktopWindowRect
    {
      .mLeft   {},
      .mRight  { desktopWindowState->mWidth },
      .mBottom { desktopWindowState->mHeight },
      .mTop    {},
    };
    return desktopWindowRect;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac

bool Tac::IsWindowHovered( const DesktopWindowHandle desktopWindowHandle )
{
  return sMouseHoveredWindowHandle == desktopWindowHandle;
}

void Tac::SetHoveredWindow( const DesktopWindowHandle desktopWindowHandle )
{
  sMouseHoveredWindowHandle = desktopWindowHandle;
}

void Tac::DesktopWindowDebugImgui()
{
  if( !ImGuiCollapsingHeader( "DesktopWindowDebugImgui" ) )
    return;

  TAC_IMGUI_INDENT_BLOCK;

  int maxNameLen {};
  for( const DesktopWindowState& state : sDesktopWindowStates )
    maxNameLen = Max( maxNameLen, state.mName.size() );


  int stateCount {};
  for( int iWindow{}; iWindow < kDesktopWindowCapacity; ++iWindow )
  {
    const DesktopWindowState* state = &sDesktopWindowStates[ iWindow ];
    if( !state->mNativeWindowHandle )
      continue;

    String handleStr{ ToString( iWindow ) };
    while( handleStr.size() < 2 )
      handleStr += ' ';

    String nameStr{ state->mName };
    while( nameStr.size() < maxNameLen )
      nameStr += ' ';

    String str;
    str += "Window: ";
    str += handleStr;
    str += ", Name: ";
    str += nameStr;
    str += ", Pos(";
    str += ToString( state->mX );
    str += ", ";
    str += ToString( state->mY );
    str += "), Size( ";
    str += ToString( state->mWidth );
    str += ", ";
    str += ToString( state->mHeight );
    str += "), Native: ";
    str += ToString( ( void* )state->mNativeWindowHandle );

    ImGuiText( str );
    stateCount++;
  }

  if( !stateCount )
    ImGuiText( " No desktop window states (how are you reading this)" );
}

