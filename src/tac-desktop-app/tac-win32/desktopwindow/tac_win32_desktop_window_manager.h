#pragma once

#include "tac-win32/tac_win32.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-engine-core/platform/tac_platform.h"

#undef FindWindow

namespace Tac
{
  struct Win32WindowManager
  {
    static void Init( Errors& );
    static void Poll( Errors& );
    static void SpawnWindow( const PlatformSpawnWindowParams&, Errors& );
    static void DespawnWindow( WindowHandle );
    static auto FindWindow( HWND ) -> WindowHandle;
    static HWND GetHWND( WindowHandle );
    static void DebugImGui();
  };
}

