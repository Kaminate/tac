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

  ExecutableStartupInfo          ExecutableStartupInfo::Init()
  {
    const ProjectFns projectFns =
    {
      .mProjectInit = StandaloneInit,
      .mProjectUpdate = StandaloneUpdate,
      .mProjectUninit = StandaloneUninit,
    };
    return ExecutableStartupInfo
    {
      .mAppName = "Vk Ex",
      .mProjectFns = projectFns,
    };
  }
} // namespace Tac

