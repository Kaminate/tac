#include "src/shell/vulkan/tac_renderer_vulkan.h" // self-inc

#include "src/common/error/tac_error_handling.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/containers/tac_map.h"
#include "src/common/assetmanagers/tac_asset.h"

#include "src/shell/tac_desktop_app.h"
#include "src/shell/vulkan/tac_vk_types.h"
#include "src/shell/tac_desktop_app_threads.h"

#include "VkBootstrap.h"

//#include <vulkan/vk_enum_string_helper.h> // string_VkResult <-- ... import std issues?

namespace Tac::Render
{
  void VkCallAux( const char* fnCallWithArgs, VkResult res, Errors& errors )
  {
    String str;
    str += fnCallWithArgs;
    str += " returned 0x";
    str += Tac::ToString( (void*)res ); // std::hex hack
    //str += String() + "(" + string_VkResult( res ) + ")";

    TAC_RAISE_ERROR( str );
  }


  static Renderer* RendererVulkanFactory()
  {
    return TAC_NEW RendererVulkan;
  }

  void RegisterRendererVulkan()
  {
    SetRendererFactory<RendererVulkan>( RendererAPI::Vulkan );
  }


  //TAC_RAISE_ERROR_RETURN();

  RendererVulkan::~RendererVulkan()
  {
    vkb::destroy_debug_utils_messenger( _instance, _debug_messenger );
    vkDestroyDevice( _device, nullptr );
    vkDestroyInstance( _instance, nullptr );
  };

  void RendererVulkan::Init( Errors& errors )
  {
    TAC_ASSERT(mGetVkExtensions);
    TAC_ASSERT(mGetVkSurfaceFn);

    vkb::InstanceBuilder builder;

    Vector<String> extensions = TAC_CALL( mGetVkExtensions( errors ));
    TAC_ASSERT(!extensions.empty()); // probably
    for( const String& extension : extensions )
    {
      //.enable_extension( VK_KHR_WIN32_SURFACE_EXTENSION_NAME )
      builder.enable_extension( extension.c_str() );
    }

    builder.set_app_name( sShellAppName );
    builder.request_validation_layers( true );
    builder.require_api_version( 1, 1, 0 );
    builder.use_default_debug_messenger();
    vkb::detail::Result<vkb::Instance> inst_ret = builder.build();
    // idk, debugger on mac shits the bed here, refuses to inspect variables or step further
    TAC_RAISE_ERROR_IF( !inst_ret, "failed to init vk" );

    vkb::Instance vkb_inst = inst_ret.value();
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;



    //use vkbootstrap to select a GPU.
    //We want a GPU that can write to the SDL surface and supports Vulkan 1.1
    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    selector.set_minimum_version( 1, 1 );
    selector.require_present( false ); // ???
    selector.prefer_gpu_device_type( vkb::PreferredDeviceType::discrete );
    selector.defer_surface_initialization();
      //.set_surface( _surface )

    vkb::detail::Result<vkb::PhysicalDevice> physicalDevice_result = selector.select();
    TAC_RAISE_ERROR_IF( !physicalDevice_result, "failed to select physical vk device" );

    vkb::PhysicalDevice physicalDevice = physicalDevice_result.value();

    //create the final Vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a Vulkan application
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;

  };
  void RendererVulkan::RenderBegin( const Frame*, Errors& ) {};
  void RendererVulkan::RenderDrawCall( const Frame*, const DrawCall*, Errors& ) {};
  void RendererVulkan::RenderEnd( const Frame*, Errors& ) {};

  OutProj RendererVulkan::GetPerspectiveProjectionAB( InProj ) { return { }; };

  void RendererVulkan::SwapBuffers() {};
  void RendererVulkan::AddBlendState( const CommandDataCreateBlendState*, Errors& ) {};
  void RendererVulkan::AddConstantBuffer( const CommandDataCreateConstantBuffer*, Errors& ) {};
  void RendererVulkan::AddDepthState( const CommandDataCreateDepthState*, Errors& ) {};
  void RendererVulkan::AddFramebuffer( const CommandDataCreateFramebuffer* data, Errors& errors )
  {
    TAC_ASSERT( DesktopAppThreads::IsMainThread() );

    const bool isWindowFramebuffer = data->mNativeWindowHandle && data->mWidth && data->mHeight;
    const bool isRenderToTextureFramebuffer = !data->mFramebufferTextures.empty();
    TAC_ASSERT( isWindowFramebuffer || isRenderToTextureFramebuffer );

    if( isWindowFramebuffer )
    {
      VkSurfaceKHR _surface;

      //VkSurfaceFn fn =  GetVkSurfaceFn();
      TAC_CALL( mGetVkSurfaceFn( _instance, data->mNativeWindowHandle, &_surface, errors ));


      vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };
      swapchainBuilder.use_default_format_selection();
      swapchainBuilder.set_desired_present_mode( VK_PRESENT_MODE_FIFO_KHR );
      swapchainBuilder.set_desired_extent( data->mWidth, data->mHeight );
      vkb::Swapchain vkbSwapchain = swapchainBuilder.build().value();

      //store swapchain and its related images
      Vector<VkImage> _swapchainImages;
      for( VkImage& img : vkbSwapchain.get_images().value() )
        _swapchainImages.push_back(img);
      
      
      Vector<VkImageView> _swapchainImageViews;
      for( VkImageView& view : vkbSwapchain.get_image_views().value())
        _swapchainImageViews.push_back(view);

      mFramebuffers[ ( int )data->mFramebufferHandle ] = {
        ._surface = _surface,
        ._swapchain = vkbSwapchain.swapchain,
        ._swapchainImageFormat = vkbSwapchain.image_format,
        ._swapchainImages = Vector( _swapchainImages.data(),
                                    _swapchainImages.data() + _swapchainImages.size() ),
        ._swapchainImageViews = Vector( _swapchainImageViews.data(),
                                        _swapchainImageViews.data() + _swapchainImageViews.size() ),
        .mDebugName = data->mStackFrame.ToString(),
      };

      mWindows[ mWindowCount++ ] = data->mFramebufferHandle;

    // get the surface of the window we opened with SDL
    //SDL_Vulkan_CreateSurface( _window, _instance, &_surface );

    }
    else if( isRenderToTextureFramebuffer )
    {
      TAC_ASSERT_INVALID_CODE_PATH;
    }
    else
    {
      TAC_ASSERT_INVALID_CODE_PATH;
    }
  }
  void RendererVulkan::AddIndexBuffer( const CommandDataCreateIndexBuffer*, Errors& ) {};
  void RendererVulkan::AddRasterizerState( const CommandDataCreateRasterizerState*, Errors& ) {};
  void RendererVulkan::AddSamplerState( const CommandDataCreateSamplerState*, Errors& ) {};
  void RendererVulkan::AddShader( const CommandDataCreateShader*, Errors& ) {};
  void RendererVulkan::AddTexture( const CommandDataCreateTexture*, Errors& ) {};
  void RendererVulkan::AddMagicBuffer( const CommandDataCreateMagicBuffer*, Errors& ) {};
  void RendererVulkan::AddVertexBuffer( const CommandDataCreateVertexBuffer*, Errors& ) {};
  void RendererVulkan::AddVertexFormat( const CommandDataCreateVertexFormat*, Errors& ) {};
  void RendererVulkan::DebugGroupBegin( StringView ) {};
  void RendererVulkan::DebugGroupEnd() {};
  void RendererVulkan::DebugMarker( StringView ) {};
  void RendererVulkan::RemoveBlendState( BlendStateHandle, Errors& ) {};
  void RendererVulkan::RemoveConstantBuffer( ConstantBufferHandle, Errors& ) {};
  void RendererVulkan::RemoveDepthState( DepthStateHandle, Errors& ) {};
  void RendererVulkan::RemoveFramebuffer( FramebufferHandle framebufferHandle, Errors& errors )
  {
    for( int i = 0; i < mWindowCount; ++i )
      if( mWindows[ i ] == framebufferHandle )
        mWindows[ i ] = mWindows[ --mWindowCount ];
    FramebufferVk* framebuffer = &mFramebuffers[ ( int )framebufferHandle ];

    vkDestroySwapchainKHR( _device,  framebuffer->_swapchain, nullptr );
    for( VkImageView view : framebuffer->_swapchainImageViews )
      vkDestroyImageView( _device, view, nullptr );

    vkDestroySurfaceKHR( _instance, framebuffer->_surface, nullptr );


    *framebuffer = FramebufferVk();
  }
  void RendererVulkan::RemoveIndexBuffer( IndexBufferHandle, Errors& ) {};
  void RendererVulkan::RemoveRasterizerState( RasterizerStateHandle, Errors& ) {};
  void RendererVulkan::RemoveSamplerState( SamplerStateHandle, Errors& ) {};
  void RendererVulkan::RemoveShader( ShaderHandle, Errors& ) {};
  void RendererVulkan::RemoveTexture( TextureHandle, Errors& ) {};
  void RendererVulkan::RemoveMagicBuffer( MagicBufferHandle, Errors& ) {};
  void RendererVulkan::RemoveVertexBuffer( VertexBufferHandle, Errors& ) {};
  void RendererVulkan::RemoveVertexFormat( VertexFormatHandle, Errors& ) {};
  void RendererVulkan::ResizeFramebuffer( const CommandDataResizeFramebuffer*, Errors& ) {};
  void RendererVulkan::SetRenderObjectDebugName( const CommandDataSetRenderObjectDebugName*, Errors& ) {};
  void RendererVulkan::UpdateConstantBuffer( const CommandDataUpdateConstantBuffer*, Errors& ) {};
  void RendererVulkan::UpdateIndexBuffer( const CommandDataUpdateIndexBuffer*, Errors& ) {};
  void RendererVulkan::UpdateTextureRegion( const CommandDataUpdateTextureRegion*, Errors& ) {};
  void RendererVulkan::UpdateVertexBuffer( const CommandDataUpdateVertexBuffer*, Errors& ) {};

  AssetPathStringView RendererVulkan::GetShaderPath(  const ShaderNameStringView& ) const
  {
    return "";
  }

  AssetPathStringView RendererVulkan::GetShaderDir() const
  {
    return "";
  }

} // namespace Tac::Render
