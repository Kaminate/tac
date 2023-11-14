#include "tac_register_renderers.h"
#include "src/common/core/tac_preprocessor.h" // Tac::asdf

#if defined( TAC_USE_RENDERER_VK ) && __has_include( "tac_renderer_vulkan.h" )
#define TAC_INCLUDED_RENDERER_VULKAN true
//#include "tac_renderer_vulkan.h"
#else
#define TAC_INCLUDED_RENDERER_VULKAN false
#endif

#if defined( TAC_USE_RENDERER_DX11 ) && __has_include( "tac_renderer_directx11.h" )
#define TAC_INCLUDED_RENDERER_DIRECTX11 true
//#include "tac_renderer_directx11.h"
#else
#define TAC_INCLUDED_RENDERER_DIRECTX11 false
#endif

// Function forward declarations
namespace Tac::Render
{
#if TAC_INCLUDED_RENDERER_VULKAN
    void RegisterRendererVulkan();
#endif

#if TAC_INCLUDED_RENDERER_DIRECTX11
    void RegisterRendererDirectX11();
#endif
}

namespace Tac
{
  // function calls
  void RegisterRenderers()
  {
#if TAC_INCLUDED_RENDERER_VULKAN
    Render::RegisterRendererVulkan();
#endif

#if TAC_INCLUDED_RENDERER_DIRECTX11
    Render::RegisterRendererDirectX11();
#endif

    asdf++;
  }
}



