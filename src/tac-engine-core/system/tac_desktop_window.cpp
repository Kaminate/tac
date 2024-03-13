#include "tac_desktop_window.h" // self-inc

#include "tac-std-lib/containers/tac_array.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/ui/imgui/tac_imgui.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/shell/tac_shell.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static DesktopWindowState sDesktopWindowStates[ kDesktopWindowCapacity ];
  static DesktopWindowHandle sMouseHoveredWindowHandle;

  v2                        DesktopWindowState::GetPosV2() const
  {
    float x = (float)mX;
    float y = (float)mY;
    return { x, y };
  }

  v2                        DesktopWindowState::GetSizeV2() const
  {
    float w = (float)mWidth;
    float h = (float)mHeight;
    return { w, h };
  }

  int  DesktopWindowRect::GetArea() const { return GetWidth() * GetHeight(); }
  bool DesktopWindowRect::IsEmpty() const { return GetArea() == 0; }
  int  DesktopWindowRect::GetWidth() const { return mRight - mLeft; }
  int  DesktopWindowRect::GetHeight() const { return mBottom - mTop; }


  bool                        IsWindowHovered( const DesktopWindowHandle desktopWindowHandle )
  {
    return sMouseHoveredWindowHandle == desktopWindowHandle;
  }

  void                        SetHoveredWindow( const DesktopWindowHandle desktopWindowHandle )
  {
    sMouseHoveredWindowHandle = desktopWindowHandle;
  }

  DesktopWindowState*         GetDesktopWindowState( const DesktopWindowHandle desktopWindowHandle )
  {
    return desktopWindowHandle.IsValid()
      ? &sDesktopWindowStates[ desktopWindowHandle.GetIndex() ]
      : nullptr;
  }

  const void* GetDesktopWindowNativeHandle( const DesktopWindowHandle hWnd )
  {
    const DesktopWindowState* state = GetDesktopWindowState(hWnd);
    return state ? state->mNativeWindowHandle : nullptr;
  }

  DesktopWindowRect           GetDesktopWindowRectScreenspace( const DesktopWindowHandle hWnd )
  {
    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( hWnd );
    const DesktopWindowRect desktopWindowRect
    {
      .mLeft = desktopWindowState->mX,
      .mRight = desktopWindowState->mX + desktopWindowState->mWidth,
      .mBottom = desktopWindowState->mY + desktopWindowState->mHeight,
      .mTop = desktopWindowState->mY,
    };
    return desktopWindowRect;
  }

  DesktopWindowRect           GetDesktopWindowRectWindowspace( DesktopWindowHandle desktopWindowHandle )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( desktopWindowHandle );
    const DesktopWindowRect desktopWindowRect
    {
      .mLeft = 0,
      .mRight = desktopWindowState->mWidth,
      .mBottom = desktopWindowState->mHeight,
      .mTop = 0,
    };
    return desktopWindowRect;
  }

  void                        DesktopWindowDebugImgui()
  {
    if( !ImGuiCollapsingHeader( "DesktopWindowDebugImgui" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    int maxNameLen = 0;
    for( const DesktopWindowState& state : sDesktopWindowStates )
      maxNameLen = Max( maxNameLen, state.mName.size() );


    int stateCount = 0;
    for( int iWindow = 0; iWindow < kDesktopWindowCapacity; ++iWindow )
    {
      const DesktopWindowState* state = &sDesktopWindowStates[ iWindow ];
      if( !state->mNativeWindowHandle )
        continue;

      ShortFixedString handleStr = ToString( iWindow );
      while( handleStr.size() < 2 )
        handleStr += ' ';

      ShortFixedString nameStr = (StringView)state->mName;
      while(nameStr.size() < maxNameLen)
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
      str += ToString( (void*)state->mNativeWindowHandle );

      ImGuiText( str );
      stateCount++;
    }

    if( !stateCount)
      ImGuiText(" No desktop window states (how are you reading this)" );
  }
} // namespace Tac

