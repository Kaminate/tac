#include "src/game-example-letter/tac_example_letter.h"

#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/tac_desktop_window.h"
#include "src/common/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"
#include "src/common/graphics/tac_font.h"

namespace Tac
{
  static DesktopWindowHandle       sDesktopWindowHandle;

  static void   ExampleLetterInitCallback( Errors& errors )
  {
    sDesktopWindowHandle = CreateTrackedWindow( "Example.Letter.Window", 50, 50, 200, 200 );
    QuitProgramOnWindowClose( sDesktopWindowHandle );

    ImGuiDebugColors();
  }

  static void   ExampleLetterUpdateCallback( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( sDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    if( KeyboardIsKeyDown( Key::Escape ) )
      OS::OSAppStopRunning();

    ImGuiSetNextWindowHandle( sDesktopWindowHandle );
    ImGuiBegin( "Example Letter" );
    FontApi::SetForceRedraw( true );
    ImGuiText( "a \n b" );
    ImGuiEnd();
  }

  ExecutableStartupInfo          ExecutableStartupInfo::Init()
  {
    return
    {
      .mAppName = "Example Letter",
      .mProjectInit = ExampleLetterInitCallback,
      .mProjectUpdate = ExampleLetterUpdateCallback,
    };
  }


} // namespace Tac
