#include "tac_register_renderers.h"

#ifdef TAC_USE_RENDERER_VK

#include "tac_renderer_vulkan.h"
static int asdf;
#endif

#ifdef TAC_USE_RENDERER_DX11
#include "tac_renderer_directx11.h"
static int fdsa;
#endif

namespace Tac
{

  // function forward declarations
  namespace Render
  {
#ifdef TAC_USE_RENDERER_VK
#if __has_include( "tac_renderer_vulkan.h" )
    void RegisterRendererVulkan();
#endif
#endif

#ifdef TAC_USE_RENDERER_DX11
#if __has_include( "tac_renderer_directx11.h" )
    void RegisterRendererDirectX11();
#endif
#endif
  }

  // function calls
  void RegisterRenderers()
  {
#ifdef TAC_USE_RENDERER_VK
#if __has_include( "tac_renderer_vulkan.h" )
    Render::RegisterRendererVulkan();
#endif
#endif

#ifdef TAC_USE_RENDERER_DX11
#if __has_include( "tac_renderer_directx11.h" )
    Render::RegisterRendererDirectX11();
#endif
#endif

  }
}



