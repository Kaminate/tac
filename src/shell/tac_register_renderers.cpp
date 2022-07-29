// the directx11.h local path is included by src/shell/windows/tacwinlib/CMakeLists.txt

#if __has_include( "tac_renderer_vulkan.h" )
#include "tac_renderer_vulkan.h"
#endif

#if __has_include( "tac_renderer_directx11.h" )
#include "tac_renderer_directx11.h"
#endif

namespace Tac
{

  void RegisterRenderers()
  {
#if __has_include( "tac_renderer_vulkan.h" )
    Render::RegisterRendererVulkan();
#endif

#if __has_include( "tac_renderer_directx11.h" )
    Render::RegisterRendererDirectX11();
#endif

  }
}



