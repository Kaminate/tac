#pragma once

/*
#include "common/graphics/tac_renderer.h"
#include "common/tac_shell.h"

#include <vulkan/vulkan.h>

struct TacVulkanVertexBuffer : public TacVertexBuffer
{
  VkBuffer mGpuBuffer;
  VkDeviceMemory mGpuMemory;
};

struct TacVulkanIndexBuffer : public TacIndexBuffer
{
  VkBuffer mGpuBuffer;
  VkDeviceMemory mGpuMemory;
};

struct TacSwapChainBuffer
{
  VkImage mImage;
  VkImageView mImageView;
  VkCommandBuffer mCommandBuffer;
  VkFence waitFence;
  VkFramebuffer mFrameBuffer;
};

struct TacVulkanRasterizerState : public TacRasterizerState
{
  VkPipelineRasterizationStateCreateInfo mRasterizationState;
};

struct TacVulkanDepthState : public TacDepthState
{
  VkPipelineDepthStencilStateCreateInfo mDepthStencilState;
};

struct TacVulkanBlendState : public TacBlendState
{
  TacVector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
  VkPipelineColorBlendStateCreateInfo colorBlendState = {};
};


struct TacVulkanGlobals
{
  TacVulkanGlobals();
  static TacVulkanGlobals* Instance();
  TacVector< const char* > mRequiredExtensions;

  void( *mCreateSurface )(
    TacShell* shell,
    VkInstance instance,
    TacDesktopWindow* desktopWindow,
    VkSurfaceKHR *psurface,
    TacErrors& errors );
};


struct TacVulkanPerWindowData : public TacRendererWindowData
{
  ~TacVulkanPerWindowData();
  void SwapBuffers( TacErrors& errors );

  VkSwapchainKHR mSwapChain = nullptr;
  TacVector< TacSwapChainBuffer > mSwapchainImages;

  VkImage mDepthStencilImage = nullptr;
  VkDeviceMemory mDepthStencilMemory = nullptr;
  VkImageView mDepthStencilImageView = nullptr;
  VkFormat mColorFormat = VK_FORMAT_UNDEFINED;
};

struct TacVulkanRenderer : public TacRenderer
{
  TacVulkanRenderer();
  ~TacVulkanRenderer();
  void Init( TacErrors& errors ) override;
  void AddVertexBuffer( TacVertexBuffer** outputVertexBuffer, TacVertexBufferData vertexBufferData, TacErrors& errors ) override;
  void AddIndexBuffer( TacIndexBuffer** outputIndexBuffer, TacIndexBufferData indexBufferData, TacErrors& errors ) override;
  void AddBlendState( TacBlendState** outputBlendState, TacBlendStateData blendStateData, TacErrors& errors ) override;
  void AddRasterizerState( TacRasterizerState** outputRasterizerState, TacRasterizerStateData rasterizerStateData, TacErrors& errors ) override;
  void AddDepthState( TacDepthState** outputDepthState, TacDepthStateData depthStateData, TacErrors& errors ) override;
  void CreateWindowContext( struct TacDesktopWindow* desktopWindow, TacErrors& errors ) override;
  uint32_t GetMemory( uint32_t typeBits, VkMemoryPropertyFlags properties, TacErrors& errors );
  void FlushCommandBuffer( VkCommandBuffer commandBuffer, TacErrors& errors );


  uint32_t mSelectedQueueFamilyIndex = 0;
  VkInstance mInstance = nullptr;
  VkDevice mDevice = nullptr;
  VkPhysicalDeviceMemoryProperties mDeviceMemoryProperties;
  VkSemaphore mSemaphoreImageAvailable = nullptr;
  VkSemaphore mSemaphoreRenderingFinished = nullptr;
  VkPhysicalDevice mPhysicalDevice = nullptr;
  VkCommandPool mCommandPool = nullptr;
  VkFormat mDepthFormat = VK_FORMAT_UNDEFINED;
  VkPipelineCache mPipelineCache = nullptr;
  VkQueue mQueue = nullptr;
  std::set< TacVulkanPerWindowData* > mPerWindowDatas;
};



void TacVulkanCallAux( TacErrors& errors, TacString functionName, VkResult res );

#define TAC_VULKAN_CALL( errors, call, ... )\
{\
  VkResult result = call( __VA_ARGS__ );\
  if( result != VK_SUCCESS )\
  {\
    TacVulkanCallAux( errors, TacStringify( call ), result );\
    TAC_HANDLE_ERROR( errors );\
  }\
}
*/

//#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_renderer_backend.h"
#include "src/common/tac_common.h"

#include <vulkan/vulkan.h>

#define TAC_VK_CALL( errors, call, ... ) {                                          \
  const VkResult result = call( __VA_ARGS__ );                                      \
  if( result )                                                                      \
  {                                                                                 \
    const char* fnCall = TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )";              \
    Tac::Render::VkCallAux( fnCall, result, errors );                               \
    TAC_HANDLE_ERROR( errors );                                                     \
  }                                                                                 \
}

namespace Tac::Render
{

  void VkCallAux( const char* fnCallWithArgs, VkResult, Errors& );

  void RegisterRendererVulkan();

  struct FramebufferVk
  {
    VkSurfaceKHR _surface{}; // Vulkan window surface
    VkSwapchainKHR _swapchain{}; // from other articles

    // image format expected by the windowing system
    VkFormat _swapchainImageFormat{};

    //array of images from the swapchain
    Vector<VkImage> _swapchainImages;

    //array of image-views from the swapchain
    Vector<VkImageView> _swapchainImageViews;

    String mDebugName;
  };

  struct RendererVulkan : public Renderer
  {
    VkInstance _instance{}; // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger{}; // Vulkan debug output handle
    VkPhysicalDevice _chosenGPU{  }; // GPU chosen as the default device
    VkDevice _device{}; // Vulkan device for commands
    Vector<String>(*mGetVkExtensions)(Errors&);
    void ( *mGetVkSurfaceFn )( VkInstance,
                                   const void* nativeWindowHandle,
                                   //const DesktopWindowHandle&,
                                   VkSurfaceKHR*,
                                   Errors& );

    ~RendererVulkan() override;
    void Init( Errors& ) override;
    void RenderBegin( const Frame*, Errors& ) override;
    void RenderDrawCall( const Frame*, const DrawCall*, Errors& ) override;
    void RenderEnd( const Frame*, Errors& ) override;

    void SwapBuffers() override;
    OutProj GetPerspectiveProjectionAB(InProj) override;
    void AddBlendState( const CommandDataCreateBlendState*, Errors& ) override;
    void AddConstantBuffer( const CommandDataCreateConstantBuffer*, Errors& ) override;
    void AddDepthState( const CommandDataCreateDepthState*, Errors& ) override;
    void AddFramebuffer( const CommandDataCreateFramebuffer*, Errors& ) override;
    void AddIndexBuffer( const CommandDataCreateIndexBuffer*, Errors& ) override;
    void AddRasterizerState( const CommandDataCreateRasterizerState*, Errors& ) override;
    void AddSamplerState( const CommandDataCreateSamplerState*, Errors& ) override;
    void AddShader( const CommandDataCreateShader*, Errors& ) override;
    void AddTexture( const CommandDataCreateTexture*, Errors& ) override;
    void AddMagicBuffer( const CommandDataCreateMagicBuffer*, Errors& ) override;
    void AddVertexBuffer( const CommandDataCreateVertexBuffer*, Errors& ) override;
    void AddVertexFormat( const CommandDataCreateVertexFormat*, Errors& ) override;
    void DebugGroupBegin( StringView ) override;
    void DebugGroupEnd() override;
    void DebugMarker( StringView ) override;
    void RemoveBlendState( BlendStateHandle, Errors& ) override;
    void RemoveConstantBuffer( ConstantBufferHandle, Errors& ) override;
    void RemoveDepthState( DepthStateHandle, Errors& ) override;
    void RemoveFramebuffer( FramebufferHandle, Errors& ) override;
    void RemoveIndexBuffer( IndexBufferHandle, Errors& ) override;
    void RemoveRasterizerState( RasterizerStateHandle, Errors& ) override;
    void RemoveSamplerState( SamplerStateHandle, Errors& ) override;
    void RemoveShader( ShaderHandle, Errors& ) override;
    void RemoveTexture( TextureHandle, Errors& ) override;
    void RemoveMagicBuffer( MagicBufferHandle, Errors& ) override;
    void RemoveVertexBuffer( VertexBufferHandle, Errors& ) override;
    void RemoveVertexFormat( VertexFormatHandle, Errors& ) override;
    void ResizeFramebuffer( const CommandDataResizeFramebuffer*, Errors& ) override;
    void SetRenderObjectDebugName( const CommandDataSetRenderObjectDebugName*, Errors& ) override;
    void UpdateConstantBuffer( const CommandDataUpdateConstantBuffer*, Errors& ) override;
    void UpdateIndexBuffer( const CommandDataUpdateIndexBuffer*, Errors& ) override;
    void UpdateTextureRegion( const CommandDataUpdateTextureRegion*, Errors& ) override;
    void UpdateVertexBuffer( const CommandDataUpdateVertexBuffer*, Errors& ) override;
    AssetPathStringView GetShaderPath(  const ShaderNameStringView& ) override;

    FramebufferVk              mFramebuffers[ kMaxFramebuffers ] = {};
    FramebufferHandle          mWindows[ kMaxFramebuffers ];
    int                        mWindowCount = 0;
  };

} // namespace Tac::Render
