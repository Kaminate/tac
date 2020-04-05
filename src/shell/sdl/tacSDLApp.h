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


struct SDLWindow;
struct SDLApp;

struct SDLWindow : public DesktopWindow
{
  ~SDLWindow();
  SDL_Window* mWindow = nullptr;
  SDLApp* app;
};

struct SDLApp : public DesktopApp
{
  ~SDLApp();
  void Init( Errors& errors ) override;
  void Poll( Errors& errors ) override;
  void SpawnWindowAux( const WindowParams& windowParams, DesktopWindow** desktopWindow, Errors& errors )override;
  void GetPrimaryMonitor( Monitor* monitor, Errors& errors ) override;
  SDLWindow* FindSDLWindowByID( Uint32 windowID );

  std::set< SDLWindow* > mWindows;
};

}
