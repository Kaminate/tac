#include "src/shell/tac_desktop_app.h" // ExecutableStartupInfo
#include "src/common/tac_error_handling.h"
#include "src/common/tac_desktop_window.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"

namespace Tac
{
  DesktopWindowHandle sDesktopWindowHandle;

  static void   StandaloneInit( Errors& errors )
  {
    sDesktopWindowHandle = CreateTrackedWindow( "Vk Example.Window" );
    TAC_HANDLE_ERROR( errors );
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

  void          ExecutableStartupInfo::Init( Errors& )
  {
    mAppName = "Vk Ex";
    mProjectInit = StandaloneInit;
    mProjectUpdate = StandaloneUpdate;
    mProjectUninit = StandaloneUninit;
  }
}

