#include "src/game-example-letter/tac_example_letter.h" // self-include

#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_os.h"
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
    ImGuiSetIsScrollbarEnabled( false );
  }

  static const char* quickbrownfox = "The quick brown fox jumps over the lazy dog";

  static void Test( float fontSize, const char* str = quickbrownfox )
  {
    ImGuiPushFontSize( fontSize );
    ImGuiText( str);
    ImGuiPopFontSize();
  }

  static void   ExampleLetterUpdateCallback( Errors& errors )
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( sDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    if( KeyboardIsKeyDown( Key::Escape ) )
      OS::OSAppStopRunning();

    ImGuiSetNextWindowHandle( sDesktopWindowHandle );
    ImGuiSetNextWindowStretch();
    ImGuiBegin( "Example Letter" );

    if( false )
    {
      static bool setForceRedraw;
      if( ImGuiCheckbox( "Force redraw", &setForceRedraw ) )
        FontApi::SetForceRedraw( setForceRedraw );
    }


    float fontSize = 10.0f;
    for( int i = 0; i < 6; ++i, fontSize *=  2 )
      Test( fontSize);


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
