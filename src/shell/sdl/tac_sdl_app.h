#include "src/common/shell/tac_shell.h"
#include "src/common/string/tac_string.h"
#include "src/common/tac_error_handling.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/tac_event.h"
#include "src/shell/tac_desktop_app.h"

#include <SDL.h>
#include <set>

namespace Tac
{


  //struct SDLWindow;
  //struct SDLApp;

  //struct SDLWindow : public DesktopWindow
  //{
  //  ~SDLWindow();
  //  void*       GetOperatingSystemHandle() override;
  //  SDL_Window* mWindow = nullptr;
  //  void*       mOperatingSystemHandle = nullptr;
  //  void*       mOperatingSystemApplicationHandle = nullptr;
  //};

  //struct SDLApp : public DesktopApp
  //{
  //  static SDLApp* Instance;
  //  SDLApp();
  //  ~SDLApp();
  //  void Init( Errors& errors ) override;
  //  void Poll( Errors& errors ) override;
  //  void SpawnWindow( DesktopWindowHandle handle,
  //                    int x,
  //                    int y,
  //                    int width,
  //                    int height ) override;
  //  void GetPrimaryMonitor( Monitor* monitor,
  //                          Errors& errors ) override;
  //  SDLWindow* FindSDLWindowByID( Uint32 windowID );

  //  std::set< SDLWindow* > mWindows;
  //};

  void SDLOSInit( Errors& );
  void SDLAppInit( Errors& );

}
