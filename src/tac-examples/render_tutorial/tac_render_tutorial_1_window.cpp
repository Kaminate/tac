#include "tac_render_tutorial_1_window.h" // self-inc

#include "tac_render_tutorial.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  static WindowHandle sWindowHandle;

  RenderTutorial1Window::RenderTutorial1Window( App::Config cfg ) : App( cfg ) {}

  void RenderTutorial1Window::Init( Errors& errors )
  {
    TAC_CALL( sWindowHandle = RenderTutorialCreateWindow( mConfig.mName, errors ) );
  }

  App* App::Create()
  {
    const App::Config config { .mName { "Hello Window" } };
    return TAC_NEW RenderTutorial1Window( config );
  };

} // namespace Tac

