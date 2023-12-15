#include "src/shell/tac_desktop_app.h" // ExecutableStartupInfo
#include "src/common/error/tac_error_handling.h"
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

  struct VKApp1Tri : public App
  {
    VKApp1Tri( const Config& cfg ) : App( cfg ) {}
    void Init( Errors& errors ) override { StandaloneInit( errors ); }
    void Update( Errors& errors ) override { StandaloneUpdate( errors ); }
    void Uninit( Errors& errors ) override{  StandaloneUninit( errors ); }
  };

  App* App::Create() { return TAC_NEW VKApp1Tri( { .mName = "Vk_ex_01_tri" } ); }

} // namespace Tac

