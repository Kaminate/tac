// the directx11.h local path is included by src/shell/windows/tacwinlib/CMakeLists.txt
#define VK_INC "tac_renderer_vulkan.h"
#define DX_INC "tac_renderer_directx11.h"
#define HAS_VK __has_include( VK_INC )
#define HAS_DX __has_include( DX_INC )

#if HAS_VK
#include VK_INC
#endif

#if HAS_DX
#include DX_INC
#endif

namespace Tac
{

  void RegisterRenderers()
  {

#if HAS_VK
    Render::RegisterRendererVulkan();
#endif

#if HAS_DX
    Render::RegisterRendererDirectX11();
#endif

  }
}



