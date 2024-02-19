#pragma once

#include "src/common/input/tac_keyboard_input.h"
#include "src/common/tac_core.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/containers/tac_list.h"
#include "src/shell/tac_iapp.h"

// Undef CreateWindow (thanks windows.h) so that we may name a function the same
#undef CreateWindow

namespace Tac
{
  struct PlatformFns;


  // -----------------------------------------------------------------------------------------------

  struct DesktopAppCreateWindowParams
  {
    const char* mName = "";
    int         mX = 0;
    int         mY = 0;
    int         mWidth = 0;
    int         mHeight = 0;
  };

  // -----------------------------------------------------------------------------------------------


  struct DesktopApp
  {
    void                Init( Errors& );
    void                Run( Errors& );
    DesktopWindowHandle CreateWindow( const DesktopAppCreateWindowParams& );
    void                DestroyWindow( const DesktopWindowHandle& );
    void                Update( Errors& );
    void                ResizeControls( const DesktopWindowHandle&, int edgePx = 7 );
    void                MoveControls( const DesktopWindowHandle&,
                                      const DesktopWindowRect& );
    void                MoveControls( const DesktopWindowHandle& );
    void                DebugImGui( Errors& );
    static DesktopApp*  GetInstance();
  };

  // -----------------------------------------------------------------------------------------------

  Errors&             GetMainErrors();

} // namespace Tac
