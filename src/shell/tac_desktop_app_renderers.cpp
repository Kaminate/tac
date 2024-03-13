#include "src/shell/tac_desktop_app_renderers.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h" // Tac::asdf
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-std-lib/dataprocess/tac_settings.h"

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


namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------

  static RendererFactory GetFactoryFromSettings()
  {
    const String name = SettingsGetString( "chosen_renderer", "" );
    for( int i = 0; i < ( int )RendererAPI::Count; ++i )
      if( ToString( ( RendererAPI )i ) == name )
        return GetRendererFactory( ( RendererAPI )i );

    return nullptr;
  }

  static RendererFactory GetFactoryFromPlatform()
  {
#if defined _WIN32 || defined _WIN64 
    return GetRendererFactory( RendererAPI::DirectX11 );
#else
    return GetRendererFactory( RendererAPI::Vulkan );
#endif
  }

  static RendererFactory GetFactoryFromIndex()
  {
    for( int i = 0; i < ( int )RendererAPI::Count; ++i )
      if( RendererFactory factory = GetRendererFactory( ( RendererAPI )i ) )
        return factory;

    return nullptr;
  }

  static RendererFactory GetFactory()
  {
    const RendererFactory factories[] =
    {
        GetFactoryFromSettings(),
        GetFactoryFromPlatform(),
        GetFactoryFromIndex(),
    };

    for( const RendererFactory factory : factories )
      if( factory )
        return factory;

    return nullptr;
  }


  // -----------------------------------------------------------------------------------------------

  // Function prototypes

#if TAC_INCLUDED_RENDERER_VULKAN
    void RegisterRendererVulkan();
#endif

#if TAC_INCLUDED_RENDERER_DIRECTX11
    void RegisterRendererDirectX11();
#endif

  // -----------------------------------------------------------------------------------------------

  static void RegisterRenderers()
  {
#if TAC_INCLUDED_RENDERER_VULKAN
    RegisterRendererVulkan();
#endif

#if TAC_INCLUDED_RENDERER_DIRECTX11
    RegisterRendererDirectX11();
#endif
  }



}


  void Tac::DesktopInitRendering( struct Errors& errors )
  {
    Render::RegisterRenderers();

    const Render::RendererFactory factory = Render::GetFactory();
    TAC_RAISE_ERROR_IF( !factory, "No renderer factories!" );

    Render::Renderer::Instance = factory();

    Render::Init( errors );
  }


