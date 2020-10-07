#include "src/common/tacShell.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacEvent.h"
#include "src/shell/tacDesktopApp.h"
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

}
