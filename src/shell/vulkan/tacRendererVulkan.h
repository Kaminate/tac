#pragma once
#include "common/graphics/tacRenderer.h"
#include "common/tacShell.h"

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
