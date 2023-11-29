#include "src/shell/tac_desktop_app.h" // ExecutableStartupInfo
#include "src/common/core/tac_error_handling.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"

namespace Tac
{
  DesktopWindowHandle sDesktopWindowHandle;

  static void   StandaloneInit( Errors& errors )
  {
    sDesktopWindowHandle = CreateTrackedWindow( "Vk Example.Window" );
  }

  static void   StandaloneUpdate( Errors& errors )
  {

    DesktopWindowState* desktopWindowState = GetDesktopWindowState( sDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

  }

  static void   StandaloneUninit( Errors& errors )
  {
  }

  void App::Init( Errors& errors ) { StandaloneInit( errors ); }
  void App::Update( Errors& errors ) { StandaloneUpdate( errors ); }
  void App::Uninit( Errors& errors ) { StandaloneUninit( errors ); }
  App App::sInstance = { .mName = "Vk_ex_01_tri" };

} // namespace Tac

