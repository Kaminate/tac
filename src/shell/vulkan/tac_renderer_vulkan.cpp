#include "tac_renderer_vulkan.h"
#include "src/common/tac_error_handling.h"

#include "vk_types.h"
#include <sstream>

#define TAC_VK_CALL( errors, call, ... )                                          \
{                                                                                 \
  VkResult result = call( __VA_ARGS__ );                                          \
  if( result )                                                                    \
  {                                                                               \
    DXGICallAux( TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )", result, errors );  \
    TAC_HANDLE_ERROR( errors );                                                   \
  }                                                                               \
}

namespace Tac
{
  namespace Render
  {

static std::map< VkResult, const char* > TacVulkanResultStrings = {
#define CASE_MACRO( vkEnumValue ) { vkEnumValue, TAC_STRINGIFY( vkEnumValue ) },
  CASE_MACRO( VK_SUCCESS )
  CASE_MACRO( VK_NOT_READY )
  CASE_MACRO( VK_TIMEOUT )
  CASE_MACRO( VK_EVENT_SET )
  CASE_MACRO( VK_EVENT_RESET )
  CASE_MACRO( VK_INCOMPLETE )
  CASE_MACRO( VK_ERROR_OUT_OF_HOST_MEMORY )
  CASE_MACRO( VK_ERROR_OUT_OF_DEVICE_MEMORY )
  CASE_MACRO( VK_ERROR_INITIALIZATION_FAILED )
  CASE_MACRO( VK_ERROR_DEVICE_LOST )
  CASE_MACRO( VK_ERROR_MEMORY_MAP_FAILED )
  CASE_MACRO( VK_ERROR_LAYER_NOT_PRESENT )
  CASE_MACRO( VK_ERROR_EXTENSION_NOT_PRESENT )
  CASE_MACRO( VK_ERROR_FEATURE_NOT_PRESENT )
  CASE_MACRO( VK_ERROR_INCOMPATIBLE_DRIVER )
  CASE_MACRO( VK_ERROR_TOO_MANY_OBJECTS )
  CASE_MACRO( VK_ERROR_FORMAT_NOT_SUPPORTED )
  CASE_MACRO( VK_ERROR_FRAGMENTED_POOL )
  CASE_MACRO( VK_ERROR_OUT_OF_POOL_MEMORY )
  CASE_MACRO( VK_ERROR_INVALID_EXTERNAL_HANDLE )
  CASE_MACRO( VK_ERROR_SURFACE_LOST_KHR )
  CASE_MACRO( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR )
  CASE_MACRO( VK_SUBOPTIMAL_KHR )
  CASE_MACRO( VK_ERROR_OUT_OF_DATE_KHR )
  CASE_MACRO( VK_ERROR_INCOMPATIBLE_DISPLAY_KHR )
  CASE_MACRO( VK_ERROR_VALIDATION_FAILED_EXT )
  CASE_MACRO( VK_ERROR_INVALID_SHADER_NV )
  CASE_MACRO( VK_ERROR_FRAGMENTATION_EXT )
  CASE_MACRO( VK_ERROR_NOT_PERMITTED_EXT )
#undef CASE_MACRO
};

    void VkCallAux( const char* fnCallWithArgs, VkResult res, Errors& errors )
    {
      std::stringstream ss;
      ss << fnCallWithArgs << " returned 0x" << std::hex << res;
      const char* inferredErrorMessage = TacVulkanResultStrings[ res ];
      if( inferredErrorMessage )
      {
        ss << "(";
        ss << inferredErrorMessage;
        ss << ")";
      }
      errors.mMessage = ss.str().c_str();
    }

    void RegisterRendererVulkan()
    {
      RendererFactoriesRegister( { RendererNameVulkan, []() { TAC_NEW RendererVulkan; } } );
      VkResult vk;
      ( void )vk;
    }


    //TAC_RAISE_ERROR_RETURN();

    RendererVulkan::~RendererVulkan(){};
    void RendererVulkan::Init( Errors& )
    {


    };
    void RendererVulkan::RenderBegin( const Render::Frame*, Errors& ) {};
    void RendererVulkan::RenderDrawCall( const Render::Frame*, const Render::DrawCall*, Errors& ) {};
    void RendererVulkan::RenderEnd( const Render::Frame*, Errors& ) {};

    void RendererVulkan::SwapBuffers() {};
    void RendererVulkan::GetPerspectiveProjectionAB( float f,
                                     float n,
                                     float& a,
                                     float& b ) {};
    void RendererVulkan::AddBlendState( Render::CommandDataCreateBlendState*, Errors& ) {};
    void RendererVulkan::AddConstantBuffer( Render::CommandDataCreateConstantBuffer*, Errors& ) {};
    void RendererVulkan::AddDepthState( Render::CommandDataCreateDepthState*, Errors& ) {};
    void RendererVulkan::AddFramebuffer( Render::CommandDataCreateFramebuffer*, Errors& ) {};
    void RendererVulkan::AddIndexBuffer( Render::CommandDataCreateIndexBuffer*, Errors& ) {};
    void RendererVulkan::AddRasterizerState( Render::CommandDataCreateRasterizerState*, Errors& ) {};
    void RendererVulkan::AddSamplerState( Render::CommandDataCreateSamplerState*, Errors& ) {};
    void RendererVulkan::AddShader( Render::CommandDataCreateShader*, Errors& ) {};
    void RendererVulkan::AddTexture( Render::CommandDataCreateTexture*, Errors& ) {};
    void RendererVulkan::AddMagicBuffer( Render::CommandDataCreateMagicBuffer*, Errors& ) {};
    void RendererVulkan::AddVertexBuffer( Render::CommandDataCreateVertexBuffer*, Errors& ) {};
    void RendererVulkan::AddVertexFormat( Render::CommandDataCreateVertexFormat*, Errors& ) {};
    void RendererVulkan::DebugGroupBegin( StringView ) {};
    void RendererVulkan::DebugGroupEnd() {};
    void RendererVulkan::DebugMarker( StringView ) {};
    void RendererVulkan::RemoveBlendState( Render::BlendStateHandle, Errors& ) {};
    void RendererVulkan::RemoveConstantBuffer( Render::ConstantBufferHandle, Errors& ) {};
    void RendererVulkan::RemoveDepthState( Render::DepthStateHandle, Errors& ) {};
    void RendererVulkan::RemoveFramebuffer( Render::FramebufferHandle, Errors& ) {};
    void RendererVulkan::RemoveIndexBuffer( Render::IndexBufferHandle, Errors& ) {};
    void RendererVulkan::RemoveRasterizerState( Render::RasterizerStateHandle, Errors& ) {};
    void RendererVulkan::RemoveSamplerState( Render::SamplerStateHandle, Errors& ) {};
    void RendererVulkan::RemoveShader( Render::ShaderHandle, Errors& ) {};
    void RendererVulkan::RemoveTexture( Render::TextureHandle, Errors& ) {};
    void RendererVulkan::RemoveMagicBuffer( Render::MagicBufferHandle, Errors& ) {};
    void RendererVulkan::RemoveVertexBuffer( Render::VertexBufferHandle, Errors& ) {};
    void RendererVulkan::RemoveVertexFormat( Render::VertexFormatHandle, Errors& ) {};
    void RendererVulkan::ResizeFramebuffer( Render::CommandDataResizeFramebuffer*, Errors& ) {};
    void RendererVulkan::SetRenderObjectDebugName( Render::CommandDataSetRenderObjectDebugName*, Errors& ) {};
    void RendererVulkan::UpdateConstantBuffer( Render::CommandDataUpdateConstantBuffer*, Errors& ) {};
    void RendererVulkan::UpdateIndexBuffer( Render::CommandDataUpdateIndexBuffer*, Errors& ) {};
    void RendererVulkan::UpdateTextureRegion( Render::CommandDataUpdateTextureRegion*, Errors& ) {};
    void RendererVulkan::UpdateVertexBuffer( Render::CommandDataUpdateVertexBuffer*, Errors& ) {};

    String RendererVulkan::GetShaderPath( StringView ) {
      return "";

    }

  } // namespace Render

} // namespace Tac
