#pragma once

#include "tac-engine-core/hid/tac_keyboard_api.h"
#include "tac-engine-core/window/tac_window_api.h"
#include "tac-std-lib/containers/tac_list.h"
#include "tac-desktop-app/tac_iapp.h"

// Undef CreateWindow (thanks windows.h) so that we may name a function the same
#undef CreateWindow

namespace Tac { struct Errors; }

namespace Tac
{
  //struct PlatformFns;

  // -----------------------------------------------------------------------------------------------

  //struct WindowApi::CreateParams
  //{
  //  const char* mName = "";
  //  int         mX = 0;
  //  int         mY = 0;
  //  int         mWidth = 0;
  //  int         mHeight = 0;
  //};

  // -----------------------------------------------------------------------------------------------


  struct DesktopApp
  {
    void                Init( Errors& );
    void                Run( Errors& );
    //WindowHandle CreateWindow( const WindowApi::CreateParams& );
    //void                DestroyWindow( const WindowHandle& );
    void                Update( Errors& );
    //void                ResizeControls( const WindowHandle&, int edgePx = 7 );
    //void                MoveControls( const WindowHandle&,
    //                                  const DesktopWindowRect& );
    //void                MoveControls( const WindowHandle& );
    void                DebugImGui( Errors& );
    static DesktopApp*  GetInstance();
  };

  // -----------------------------------------------------------------------------------------------

  Errors&             GetMainErrors();

} // namespace Tac
