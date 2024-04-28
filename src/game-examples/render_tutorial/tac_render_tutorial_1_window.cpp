//#include "src/common/containers/tac_array.h"
//#include "src/common/error/tac_error_handling.h"
//#include "src/common/math/tac_math.h"
//#include "src/common/preprocess/tac_preprocessor.h"
//#include "src/common/shell/tac_shell_timestep.h"
#include "tac-std-lib/os/tac_os.h"
//

#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle

//#include "src/shell/tac_desktop_window_settings_tracker.h"
//#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h"
//#include "src/shell/windows/tac_win32.h"

#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-engine-core/window/tac_sys_window_api.h"

namespace Tac
{
  static WindowHandle sWindowHandle;

  struct RenderTutorial1Window : public App
  {
    RenderTutorial1Window( App::Config cfg ) : App( cfg ) {}
    void Init( InitParams initParams, Errors& errors ) override
    {
      const SysWindowApi* windowApi = initParams.mWindowApi;
      const Monitor monitor = OS::OSGetPrimaryMonitor();
      const v2i windowSize{ monitor.mSize / 2 };
      const v2i windowPos{ ( monitor.mSize - windowSize ) / 2 };
      const WindowCreateParams windowCreateParams
      {
        .mName { "Hello Window" },
        .mPos  { windowPos },
        .mSize { windowSize },
      };
      sWindowHandle = windowApi->CreateWindow( windowCreateParams, errors );
    }
  };

  App* App::Create()
  {
    const App::Config config { .mName { "Hello Window" } };
    return TAC_NEW RenderTutorial1Window( config );
  };

} // namespace Tac

