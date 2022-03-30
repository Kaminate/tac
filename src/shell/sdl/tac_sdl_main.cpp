#include "src/shell/sdl/tac_sdl_app.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_os.h"

#if __has_include("src/common/tac_os.h")
#include "src/common/tac_os.h"
#endif

#if __has_include("src/shell/vulkan/tac_renderer_vulkan.h")
#include "src/shell/vulkan/tac_renderer_vulkan.h"
#endif

#if __has_include("tac_renderer_directx11.h")
// #if __has_include("windows/tacwinlib/renderer/tac_renderer_directx11.h")
// #if __has_include("shell/windows/tacwinlib/renderer/tac_renderer_directx11.h")
// #if __has_include("src/shell/windows/tacwinlib/renderer/tac_renderer_directx11.h")
// #include "src/shell/windows/tacwinlib/renderer/tac_renderer_directx11.h"
// #include "shell/windows/tacwinlib/renderer/tac_renderer_directx11.h"
#include "tac_renderer_directx11.h"
// #include "windows/tacwinlib/renderer/tac_renderer_directx11.h"
#endif

// Exists a better way?
#if _WIN32
#include "src/shell/windows/tacwinlib/renderer/tac_renderer_directx11.h"
#endif

using namespace Tac;

void mainAux( Errors& errors )
{
  SDLOSInit( errors );
  TAC_HANDLE_ERROR( errors );

#if __has_include("src/shell/windows/tacwinlib/renderer/tac_renderer_directx11.h")
  Render::RegisterRendererDirectX11();
#endif

#if _WIN32
  Render::RegisterRendererDirectX11();
#endif

  SDLAppInit( errors );
  TAC_HANDLE_ERROR( errors );

  DesktopAppRun( errors );
  TAC_HANDLE_ERROR( errors );
}

int main( int, char ** )
{
  mainAux( *GetMainErrors() );
  DesktopAppReportErrors();

  return 0;
}


