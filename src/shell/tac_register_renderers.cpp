// the directx11.h local path is included by src/shell/windows/tacwinlib/CMakeLists.txt

namespace Tac
{

  namespace Render
  {
#if __has_include( "tac_renderer_vulkan.h" )
    void RegisterRendererVulkan();
#endif

#if __has_include( "tac_renderer_directx11.h" )
    void RegisterRendererDirectX11();
#endif
  }

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



