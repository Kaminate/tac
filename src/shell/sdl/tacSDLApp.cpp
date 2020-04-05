#include "src/shell/sdl/tacSDLApp.h"
#include "src/common/tacOS.h"
#include <SDL_syswm.h>

namespace Tac
{


SDLWindow::~SDLWindow()
{
  SDL_DestroyWindow( mWindow );
  app->mWindows.erase( this );
}

SDLApp::~SDLApp()
{
}
void SDLApp::Init( Errors& errors )
{
  int sdl_init_result = SDL_Init( SDL_INIT_EVERYTHING );
  if( sdl_init_result )
  {
    errors = SDL_GetError();
    return;
  }
}
void SDLApp::Poll( Errors& errors )
{
  SDL_Event event;
  while( SDL_PollEvent( &event ) )
  {
    if( OS::Instance->mShouldStopRunning )
      break;
    switch( event.type )
    {
    case SDL_QUIT:
    {
      OS::Instance->mShouldStopRunning = true;
    } break;
    case SDL_WINDOWEVENT:
    {
      SDLWindow* sdlWindow = FindSDLWindowByID( event.window.windowID );
      switch( event.window.event )
      {
      case SDL_WINDOWEVENT_CLOSE:
      {
        delete sdlWindow;
      } break;
      case SDL_WINDOWEVENT_RESIZED:
      {
        sdlWindow->mWidth = ( int )event.window.data1;
        sdlWindow->mHeight = ( int )event.window.data2;
        sdlWindow->mRendererData->OnResize( errors );
      } break;
      }
    } break;
    }
  }
}
void SDLApp::GetPrimaryMonitor( Monitor* monitor, Errors& errors )
{
  SDL_Rect rect;
  if( SDL_GetDisplayBounds( 0, &rect ) )
  {
    errors = va( "Failed to get display bounds %s", SDL_GetError() );
    TAC_HANDLE_ERROR( errors );
  }
  monitor->w = rect.w;
  monitor->h = rect.h;
}
SDLWindow* SDLApp::FindSDLWindowByID( Uint32 windowID )
{
  for( SDLWindow* linuxWindow : mWindows )
  {
    if( SDL_GetWindowID( linuxWindow->mWindow ) == windowID )
    {
      return linuxWindow;
    }
  }
  return nullptr;
}
void SDLApp::SpawnWindowAux( const WindowParams& windowParams, DesktopWindow** desktopWindow, Errors& errors )
{
  Uint32 flags =
    SDL_WINDOW_SHOWN |
    SDL_WINDOW_RESIZABLE |
    //SDL_WINDOW_BORDERLESS |
    0;
  SDL_Window* sdlWindow = SDL_CreateWindow(
    windowParams.mName.c_str(),
    windowParams.mX,
    windowParams.mY,
    windowParams.mWidth,
    windowParams.mHeight,
    flags
  );
  SDL_RaiseWindow( sdlWindow );

  void* operatingSystemHandle = nullptr;
  void* operatingSystemApplicationHandle = nullptr;
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
  SDL_SysWMinfo wmInfo;
  SDL_VERSION( &wmInfo.version );
  if( SDL_FALSE == SDL_GetWindowWMInfo( sdlWindow, &wmInfo ) )
  {
    errors = "Failed to get sdl window wm info";
    TAC_HANDLE_ERROR( errors );
  }
  operatingSystemHandle = wmInfo.info.win.window;
  operatingSystemApplicationHandle = wmInfo.info.win.hinstance;
#endif

  auto linuxWindow = new SDLWindow();
  linuxWindow->mWindow = sdlWindow;
  linuxWindow->app = this;
  linuxWindow->mOperatingSystemHandle = operatingSystemHandle;
  linuxWindow->mOperatingSystemApplicationHandle = operatingSystemApplicationHandle;
  *( WindowParams* )linuxWindow = windowParams;
  *desktopWindow = linuxWindow;
  mWindows.insert( linuxWindow );
}
}
