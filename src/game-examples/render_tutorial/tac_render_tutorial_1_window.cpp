#include "tac_render_tutorial.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  static WindowHandle sWindowHandle;

  struct RenderTutorial1Window : public App
  {
    RenderTutorial1Window( App::Config cfg ) : App( cfg ) {}

    void Init( InitParams initParams, Errors& errors ) override
    {
      TAC_CALL( sWindowHandle = RenderTutorialCreateWindow(
        initParams.mWindowApi, "Hello Window", errors ) );
    }
  };

  App* App::Create()
  {
    const App::Config config { .mName { "Hello Window" } };
    return TAC_NEW RenderTutorial1Window( config );
  };

} // namespace Tac

