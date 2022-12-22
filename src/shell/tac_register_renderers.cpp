// the directx11.h local path is included by src/shell/windows/CMakeLists.txt

namespace Tac
{

  // function forward declarations
  namespace Render
  {
#if __has_include( "tac_renderer_vulkan.h" )
    void RegisterRendererVulkan();
#endif

#if __has_include( "tac_renderer_directx11.h" )
    void RegisterRendererDirectX11();
#endif
  }

  // function calls
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



