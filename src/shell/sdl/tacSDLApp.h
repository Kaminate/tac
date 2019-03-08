#include "common/tacShell.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"
#include "common/tacEvent.h"
#include "shell/tacDesktopApp.h"

#include <SDL.h>

#include <set>

struct TacSDLWindow;
struct TacSDLApp;

struct TacSDLWindow : public TacDesktopWindow
{
  ~TacSDLWindow();
  SDL_Window* mWindow = nullptr;
  TacSDLApp* app;
};

struct TacSDLApp : public TacDesktopApp
{
  ~TacSDLApp();
  void Init( TacErrors& errors ) override;
  void Poll( TacErrors& errors ) override;
  void SpawnWindowAux( const TacWindowParams& windowParams, TacDesktopWindow** desktopWindow, TacErrors& errors )override;
  void GetPrimaryMonitor( TacMonitor* monitor, TacErrors& errors ) override;
  TacSDLWindow* FindSDLWindowByID( Uint32 windowID );

  std::set< TacSDLWindow* > mWindows;
};

