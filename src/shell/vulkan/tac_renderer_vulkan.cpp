/*
#include "tacRendererVulkan.h"
#include "common/tac_desktop_window.h"
#include "common/tac_shell.h"
#include "common/tac_algorithm.h"
#include "common/tac_os.h"

#include <map>

struct TacVulkanRenderer;
struct TacVulkanPerWindowData;


static std::map< TacFillMode, VkPolygonMode > GetPolygonMode = {
  { TacFillMode::Solid, VK_POLYGON_MODE_FILL },
{ TacFillMode::Wireframe, VK_POLYGON_MODE_LINE },
};
static std::map< TacCullMode, VkCullModeFlags > GetCullMode = {
  { TacCullMode::None, VK_CULL_MODE_NONE },
{ TacCullMode::Back, VK_CULL_MODE_BACK_BIT },
{ TacCullMode::Front, VK_CULL_MODE_FRONT_BIT },
};
static std::map< TacDepthFunc, VkCompareOp > TacVulkanCompareOp = {
  { TacDepthFunc::Less, VK_COMPARE_OP_LESS },
{ TacDepthFunc::LessOrEqual, VK_COMPARE_OP_LESS_OR_EQUAL },
};
static std::map< VkPhysicalDeviceType, TacString > TacVulkanPhysicalDeviceTypeStrings = {
  { VK_PHYSICAL_DEVICE_TYPE_OTHER, "other" },
{ VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, "integrated gpu" },
{ VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "discrete gpu" },
{ VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, "virtual gpu" },
{ VK_PHYSICAL_DEVICE_TYPE_CPU, "cpu" },
};
static std::map< VkResult, TacString > TacVulkanResultStrings = {
#define CASE_MACRO( vkEnumValue ) { vkEnumValue, TacStringify( vkEnumValue ) },
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

static VkFrontFace GetFrontFace( bool frontCounterClockwise )
{
  if( frontCounterClockwise )
    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
  return VK_FRONT_FACE_CLOCKWISE;
}

static VkBool32 TacVulkanBool( bool b )
{
  return b ? VK_TRUE : VK_FALSE;
}

static VkBool32 TacOnVulkanDebugReport(
  VkDebugReportFlagsEXT flags,
  VkDebugReportObjectTypeEXT objectType,
  uint64_t object,
  size_t location,
  int32_t messageCode,
  const char* pLayerPrefix,
  const char* pMessage,
  void* pUserData )
{
  auto shell = ( TacShell* )pUserData;
  shell->mLogData.EmitEvent( pMessage );
  TacOS::Instance->DebugBreak();
  VkBool32 shouldAbortVkCallThatTriggeredDebugReport = VK_FALSE;
  return shouldAbortVkCallThatTriggeredDebugReport;
}


  TacVulkanPerWindowData::~TacVulkanPerWindowData()
  {
  }

  void TacVulkanPerWindowData::SwapBuffers( TacErrors& errors )
  {
    auto renderer = ( TacVulkanRenderer* )mRenderer;
    VkDevice device = renderer->mDevice;
    VkSemaphore mSemaphoreRenderingFinished = renderer->mSemaphoreRenderingFinished;
    VkSemaphore mSemaphoreImageAvailable = renderer->mSemaphoreImageAvailable;
    VkQueue queue = renderer->mQueue;


    uint32_t imageIndex;

    // Get next image in the swap chain (back/front buffer)
    // By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
    // With that we don't have to handle VK_NOT_READY
    VkSwapchainKHR swapChain = mSwapChain;
    TAC_VULKAN_CALL( errors, vkAcquireNextImageKHR, device, swapChain, UINT64_MAX, mSemaphoreImageAvailable, ( VkFence )nullptr, &imageIndex );

    TacSwapChainBuffer* currentSwapChainBuffer = &mSwapchainImages[ imageIndex ];

    // Use a fence to wait until the command buffer has finished execution before using it again
    TacVector< VkFence > fences = { currentSwapChainBuffer->waitFence };
    TAC_VULKAN_CALL( errors, vkWaitForFences, device, fences.size(), fences.data(), VK_TRUE, UINT64_MAX );
    TAC_VULKAN_CALL( errors, vkResetFences, device, fences.size(), fences.data() );

    // Pointer to the list of pipeline stages that the semaphore waits will occur at
    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;


    // Semaphore(s) to be signaled when command buffers have completed
    TacVector< VkSemaphore > signalSemaphores = { mSemaphoreRenderingFinished };

    // Semaphore(s) to wait upon before the submitted command buffer starts executing
    TacVector< VkSemaphore > waitSemaphores = { mSemaphoreImageAvailable };

    // Command buffers(s) to execute in this batch (submission)
    TacVector< VkCommandBuffer > commandBuffers = { currentSwapChainBuffer->mCommandBuffer };

    // The submit info structure specifices a command buffer queue submission batch
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStageMask;
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pSignalSemaphores = signalSemaphores.data();
    submitInfo.signalSemaphoreCount = signalSemaphores.size();;
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.commandBufferCount = commandBuffers.size();

    TacVector<VkSubmitInfo > submitInfos = { submitInfo };

    // Submit to the graphics queue passing a wait fence
    TAC_VULKAN_CALL( errors, vkQueueSubmit, queue, submitInfos.size(), submitInfos.data(), currentSwapChainBuffer->waitFence );


    // Present the current buffer to the swap chain
    // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
    // This ensures that the image is not presented to the windowing system until all commands have been submitted

    TacVector< VkSwapchainKHR > swapchains = { swapChain };

    waitSemaphores = { mSemaphoreRenderingFinished };
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.swapchainCount = swapchains.size();;
    presentInfo.pSwapchains = swapchains.data();
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pWaitSemaphores = waitSemaphores.data();
    presentInfo.waitSemaphoreCount = waitSemaphores.size();
    TAC_VULKAN_CALL( errors, vkQueuePresentKHR, queue, &presentInfo );
  }

  TacVulkanRenderer::TacVulkanRenderer()
  {
  }

  TacVulkanRenderer::~TacVulkanRenderer()
  {
    // todo
    TacErrors errors;

    // clean things up ourselves to ensure proper order and avoid exit crash
    if( mDevice )
    {
      TAC_VULKAN_CALL( errors, vkDeviceWaitIdle, mDevice );
      vkDestroyDevice( mDevice, nullptr );
    }
    if( mInstance )
    {
      vkDestroyInstance( mInstance, nullptr );
    }
  }

  void TacVulkanRenderer::Init( TacErrors& errors ) 
  {
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pNext;
    application_info.pApplicationName = mShell->mAppName.c_str();
    application_info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    application_info.pEngineName = mShell->mAppName.c_str();
    application_info.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    application_info.apiVersion = VK_MAKE_VERSION( 1, 0, 0 );

    uint32_t propertyCount;
    TAC_VULKAN_CALL( errors, vkEnumerateInstanceLayerProperties, &propertyCount, nullptr );
    TacVector< VkLayerProperties > properties( propertyCount );
    TAC_VULKAN_CALL( errors, vkEnumerateInstanceLayerProperties, &propertyCount, properties.data() );

    bool isDebuggingVulkan = TacIsDebugMode();

    TacVector< const char* > enabledLayerNames;
    if( isDebuggingVulkan )
    {
      std::set< TacString > debugLayerNames =
      {
        // "best practices"
        // ( commented out because sasha doesnt use this )
        // "VK_LAYER_LUNARG_assistant_layer",

        // outputs the frames - per - second of the target application in the applications title bar
        // ( commented out because sasha doesnt use this )
        // "VK_LAYER_LUNARG_monitor",

        // Automatically loads the following other layers ( recommended )
        // ( on mobile it doesn't? )
        "VK_LAYER_LUNARG_standard_validation",

        // - VK_LAYER_GOOGLE_threading
        // - VK_LAYER_LUNARG_parameter_validation - validate api parameter values
        // - VK_LAYER_LUNARG_device_limits
        //   Track all Vulkan objects and flag invalid objects and object memory leaks
        // - VK_LAYER_LUNARG_object_tracker
        // - VK_LAYER_LUNARG_image
        //
        //   Validates state that feeds into a Draw call
        //   ( descriptor sets, command buffers, pipeline states, and dynamic states )
        // - VK_LAYER_LUNARG_core_validation
        // - VK_LAYER_LUNARG_swapchain
        // - VK_LAYER_GOOGLE_unique_objects

        // "VK_LAYER_NV_optimus",
        // "VK_LAYER_NV_nsight",
        // "VK_LAYER_VALVE_steam_overlay",
        // "VK_LAYER_RENDERDOC_Capture",

        //  This debugging layer prints API calls, parameters, and values to the identified output stream.
        // "VK_LAYER_LUNARG_api_dump"
      };
      bool loadEverything = false;
      for( VkLayerProperties& property : properties )
      {
        if( loadEverything || TacContains( debugLayerNames, property.layerName ) )
        {
          enabledLayerNames.push_back( property.layerName );
        }
      }
    }

    uint32_t extensionPropertyCount;
    TAC_VULKAN_CALL( errors, vkEnumerateInstanceExtensionProperties, nullptr, &extensionPropertyCount, nullptr );
    TacVector< VkExtensionProperties > extensionProperties( extensionPropertyCount );
    TAC_VULKAN_CALL( errors, vkEnumerateInstanceExtensionProperties, nullptr, &extensionPropertyCount, extensionProperties.data() );
    auto IsValidExtension = [ & ]( const char* extensionName )
    {
      for( VkExtensionProperties prop : extensionProperties )
        if( TacStrCmp( prop.extensionName, extensionName ) == 0 )
          return true;
      return false;
    };

    TacVulkanGlobals* vg = TacVulkanGlobals::Instance();
    if( isDebuggingVulkan )
      vg->mRequiredExtensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

    TacVector< const char* > extensionNames;
    for( const char* requiredExtension : vg->mRequiredExtensions )
    {
      if( !IsValidExtension( requiredExtension ) )
      {
        errors += "Failed to get extension ";
        errors += requiredExtension;
        return;
      }
      extensionNames.push_back( requiredExtension );
    }

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext;
    instance_create_info.flags;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = ( uint32_t )enabledLayerNames.size();
    instance_create_info.ppEnabledLayerNames = enabledLayerNames.data();
    instance_create_info.enabledExtensionCount = ( uint32_t )extensionNames.size();
    instance_create_info.ppEnabledExtensionNames = extensionNames.data();


    const VkAllocationCallbacks* allocator = nullptr;
    TAC_VULKAN_CALL( errors, vkCreateInstance, &instance_create_info, allocator, &mInstance );

    if( isDebuggingVulkan )
    {
      auto vkCreateDebugReportCallbackEXT =
        ( PFN_vkCreateDebugReportCallbackEXT )
        ( vkGetInstanceProcAddr( mInstance, "vkCreateDebugReportCallbackEXT" ) );
      auto vkDebugReportMessageEXT =
        ( PFN_vkDebugReportMessageEXT )
        ( vkGetInstanceProcAddr( mInstance, "vkDebugReportMessageEXT" ) );
      auto vkDestroyDebugReportCallbackEXT =
        ( PFN_vkDestroyDebugReportCallbackEXT )
        ( vkGetInstanceProcAddr( mInstance, "vkDestroyDebugReportCallbackEXT" ) );



      VkDebugReportFlagsEXT debugReportFlags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

      VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
      dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
      dbgCreateInfo.pfnCallback = TacOnVulkanDebugReport;
      dbgCreateInfo.flags = debugReportFlags;
      dbgCreateInfo.pUserData = mShell;

      VkDebugReportCallbackEXT callback;
      TAC_VULKAN_CALL( errors, vkCreateDebugReportCallbackEXT,
        mInstance,
        &dbgCreateInfo,
        nullptr,
        &callback );
    }

    uint32_t physicalDeviceCount;
    TAC_VULKAN_CALL( errors, vkEnumeratePhysicalDevices, mInstance, &physicalDeviceCount, nullptr );
    if( !physicalDeviceCount )
    {
      errors = "No physical vk devices found";
      return;
    }
    TacVector< VkPhysicalDevice > physicalDevices( physicalDeviceCount );
    TAC_VULKAN_CALL( errors, vkEnumeratePhysicalDevices, mInstance, &physicalDeviceCount, physicalDevices.data() );
    TacVector< const char* > requiredDeviceExtensionNames =
    {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    TacVector< const char* > deviceExtensionNames;


    for( VkPhysicalDevice otherphysicalDevice : physicalDevices )
    {
      VkPhysicalDeviceProperties deviceProperties;
      vkGetPhysicalDeviceProperties( otherphysicalDevice, &deviceProperties );

      if( TacIsDebugMode() )
      {
        uint32_t apiVersionMajor = deviceProperties.apiVersion >> 22;
        uint32_t apiVersionMinor = deviceProperties.apiVersion & 0xfff;
        TacString apiVersion;
        apiVersion += TacToString( ( int )apiVersionMajor );
        apiVersion += ".";
        apiVersion += TacToString( ( int )apiVersionMinor );

        TacVector< TacString > lines =
        {
          TacString( "Device name: " ) + deviceProperties.deviceName,
          TacString( "Device type: " ) + TacToString( deviceProperties.deviceType ),
          TacString( "Device vk api ver: " ) + apiVersion
        };
        TacString toLog = TacSeparateNewline( lines );
        mShell->mLogData.EmitEvent( toLog );
      }

      mPhysicalDevice = otherphysicalDevice;
    }
    if( !mPhysicalDevice )
    {
      errors = "no good physical device";
      return;
    }

    vkGetPhysicalDeviceMemoryProperties( mPhysicalDevice, &mDeviceMemoryProperties );

    uint32_t deviceExtensionPropertyCount;
    TAC_VULKAN_CALL( errors, vkEnumerateDeviceExtensionProperties, mPhysicalDevice, nullptr, &deviceExtensionPropertyCount, nullptr );
    TacVector< VkExtensionProperties > deviceExtensionProperties( deviceExtensionPropertyCount );
    TAC_VULKAN_CALL( errors, vkEnumerateDeviceExtensionProperties, mPhysicalDevice, nullptr, &deviceExtensionPropertyCount, deviceExtensionProperties.data() );

    auto HasDeviceExtension = [ & ]( const char* deviceExtensionName )
    {
      for( VkExtensionProperties prop : deviceExtensionProperties )
        if( TacStrCmp( prop.extensionName, deviceExtensionName ) == 0 )
          return true;
      return false;
    };

    for( const char* name : requiredDeviceExtensionNames )
    {
      if( !HasDeviceExtension( name ) )
      {
        errors = TacString( "missing ext" ) + name;
        return;
      }
      deviceExtensionNames.push_back( name );
    }
    for( const char* availableIfDebuggerAttached : { VK_EXT_DEBUG_MARKER_EXTENSION_NAME } )
    {
      if( HasDeviceExtension( availableIfDebuggerAttached ) )
      {
        deviceExtensionNames.push_back( availableIfDebuggerAttached );
      }
    }

    //queueFamilyIndex is an unsigned integer indicating the index of the queue family to create on this device.
    // This index corresponds to the index of an element of the pQueueFamilyProperties array that was returned by

    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties( mPhysicalDevice, &queueFamilyPropertyCount, nullptr );
    if( !queueFamilyPropertyCount )
    {
      errors = "???";
      return;
    }
    TacVector< VkQueueFamilyProperties > queueFamilyProperties( queueFamilyPropertyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( mPhysicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data() );

    VkQueueFlagBits queueBits = VK_QUEUE_GRAPHICS_BIT;
    // one day, add
    // VK_QUEUE_TRANSFER_BIT,
    // VK_QUEUE_COMPUTE_BIT
    // VK_QUEUE_GRAPHICS_BIT
    // ( which may require additional queues )
    bool selectedQueueFamilyIndexFound = false;
    for( uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyPropertyCount; ++queueFamilyIndex )
    {
      VkQueueFamilyProperties queueFamilyProperty = queueFamilyProperties[ queueFamilyIndex ];
      if( queueFamilyProperty.queueFlags & queueBits )
      {
        mSelectedQueueFamilyIndex = queueFamilyIndex;
        selectedQueueFamilyIndexFound = true;
      }
    }

    TacVector<float> queue_priorities = { 1.0f }; // { 0.0f }?

    TacVector< VkDeviceQueueCreateInfo > queue_create_infos;

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pNext = nullptr;
    queue_create_info.flags = 0;
    queue_create_info.queueFamilyIndex = mSelectedQueueFamilyIndex;
    queue_create_info.queueCount = ( uint32_t )queue_priorities.size();
    queue_create_info.pQueuePriorities = queue_priorities.data();
    queue_create_infos.push_back( queue_create_info );

    VkPhysicalDeviceFeatures enabledFeatures = {};
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = queue_create_infos.size();
    deviceCreateInfo.pQueueCreateInfos = queue_create_infos.data();
    deviceCreateInfo.enabledExtensionCount = ( uint32_t )deviceExtensionNames.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames.data();
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;


    TAC_VULKAN_CALL( errors, vkCreateDevice, mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice );

    VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = mSelectedQueueFamilyIndex;
    cmdPoolInfo.flags = createFlags;
    VkCommandPool cmdPool;
    TAC_VULKAN_CALL( errors, vkCreateCommandPool, mDevice, &cmdPoolInfo, nullptr, &cmdPool );
    mCommandPool = cmdPool;

    VkQueue queue;
    vkGetDeviceQueue( mDevice, mSelectedQueueFamilyIndex, 0, &queue );
    mQueue = queue;

    bool formatFound = false;
    VkFormat depthFormat;
    auto formats =
    {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM
    };
    for( VkFormat format : formats )
    {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties( mPhysicalDevice, format, &props );
      if( props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
      {
        depthFormat = format;
        formatFound = true;
        break;
      }
    }
    if( !formatFound )
    {
      errors = "failed to get depth format";
      return;
    }


    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    TAC_VULKAN_CALL( errors, vkCreateSemaphore, mDevice, &semaphore_create_info, nullptr, &mSemaphoreImageAvailable );

    TAC_VULKAN_CALL( errors, vkCreateSemaphore, mDevice, &semaphore_create_info, nullptr, &mSemaphoreRenderingFinished );

    VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &submitPipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &mSemaphoreImageAvailable;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &mSemaphoreRenderingFinished;

    TacVector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM
    };
    for( auto& format : depthFormats )
    {
      VkFormatProperties formatProps;
      vkGetPhysicalDeviceFormatProperties( mPhysicalDevice, format, &formatProps );
      if( formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
      {
        mDepthFormat = format;
        break;
      }
    }
    if( mDepthFormat == VK_FORMAT_UNDEFINED )
    {
      errors = "Failed to find acceptable depth format";
      TAC_HANDLE_ERROR( errors );
      return;
    }



    VkPipelineCache pipelineCache;
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    TAC_VULKAN_CALL( errors, vkCreatePipelineCache, mDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache );
    mPipelineCache = pipelineCache;

  }

  void TacVulkanRenderer::AddVertexBuffer( TacVertexBuffer** outputVertexBuffer, TacVertexBufferData vertexBufferData, TacErrors& errors ) 
  {

    TacAccess access = vertexBufferData.access;
    const void* optionalData = vertexBufferData.optionalData;
    int vertexCount = vertexBufferData.mNumVertexes;
    int strideBytesBetweenVertexes = vertexBufferData.mStrideBytesBetweenVertexes;
    const TacString& debugName = vertexBufferData.mName;

    VkDeviceSize size = vertexCount * strideBytesBetweenVertexes;

    VkBuffer cpuBuffer;
    {
      VkBufferCreateInfo bufferCreateInfo = {};
      bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferCreateInfo.size = size;
      bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;// Buffer is used as the copy source
      TAC_VULKAN_CALL( errors, vkCreateBuffer, mDevice, &bufferCreateInfo, nullptr, &cpuBuffer );
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements( mDevice, cpuBuffer, &memReqs );


    VkDeviceMemory cpuMemory;
    {
      VkMemoryPropertyFlags properties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | // <-- so we ( the cpu ) can copy our data here
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // <-- so our writes are visible to the GPU after unmapping

      VkMemoryAllocateInfo memAlloc = {};
      memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = GetMemory( memReqs.memoryTypeBits, properties, errors );
      TAC_HANDLE_ERROR( errors );
      TAC_VULKAN_CALL( errors, vkAllocateMemory, mDevice, &memAlloc, nullptr, &cpuMemory );
    }

    if( optionalData )
    {
      void* mappedData;
      TAC_VULKAN_CALL( errors, vkMapMemory, mDevice, cpuMemory, 0, size, 0, &mappedData );
      TacMemCpy( mappedData, optionalData, ( int )size );
      vkUnmapMemory( mDevice, cpuMemory );
    }

    TAC_VULKAN_CALL( errors, vkBindBufferMemory, mDevice, cpuBuffer, cpuMemory, 0 );


    VkBuffer gpuBuffer;
    {
      VkBufferCreateInfo bufferCreateInfo = {};
      bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferCreateInfo.size = size;
      bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
      TAC_VULKAN_CALL( errors, vkCreateBuffer, mDevice, &bufferCreateInfo, nullptr, &gpuBuffer );
    }

    vkGetBufferMemoryRequirements( mDevice, gpuBuffer, &memReqs );

    VkDeviceMemory gpuMemory;
    {
      VkMemoryAllocateInfo memAlloc = {};
      memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = GetMemory( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, errors );
      TAC_HANDLE_ERROR( errors );
      TAC_VULKAN_CALL( errors, vkAllocateMemory, mDevice, &memAlloc, nullptr, &gpuMemory );
    }
    TAC_VULKAN_CALL( errors, vkBindBufferMemory, mDevice, gpuBuffer, gpuMemory, 0 );

    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = nullptr;

    VkCommandBuffer cmdBuffer;
    {
      VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
      cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      cmdBufAllocateInfo.commandPool = mCommandPool;
      cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      cmdBufAllocateInfo.commandBufferCount = 1;

      TAC_VULKAN_CALL( errors, vkAllocateCommandBuffers, mDevice, &cmdBufAllocateInfo, &cmdBuffer );
    }

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    TAC_VULKAN_CALL( errors, vkBeginCommandBuffer, cmdBuffer, &cmdBufInfo );

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer( cmdBuffer, cpuBuffer, gpuBuffer, 1, &copyRegion );


    // Flushing the command buffer will also submit it to the queue and uses a fence to ensure that all commands have been executed before returning
    FlushCommandBuffer( cmdBuffer, errors );

    // Destroy staging buffers, ( can only be done after copies have been submitted and executed )
    vkDestroyBuffer( mDevice, cpuBuffer, nullptr );
    vkFreeMemory( mDevice, cpuMemory, nullptr );

    auto vertexBuffer = new TacVulkanVertexBuffer();
    vertexBuffer->mNumVertexes = vertexCount;
    vertexBuffer->mStrideBytesBetweenVertexes = strideBytesBetweenVertexes;
    vertexBuffer->mGpuBuffer = gpuBuffer;
    vertexBuffer->mGpuMemory = gpuMemory;
    vertexBuffer->mName = debugName;

    *outputVertexBuffer = vertexBuffer;
  }

  void TacVulkanRenderer::AddIndexBuffer( TacIndexBuffer** outputIndexBuffer, TacIndexBufferData indexBufferData, TacErrors& errors ) 
  {
    TacAccess access = indexBufferData.access;
    const void* data = indexBufferData.data;
    int indexCount = indexBufferData.indexCount;
    TacFormat dataType = indexBufferData.dataType;
    const TacString& debugName = indexBufferData.mName;

    VkDeviceSize size = indexCount * dataType.mPerElementByteCount;

    VkBuffer cpuBuffer;
    {
      VkBufferCreateInfo bufferCreateInfo = {};
      bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferCreateInfo.size = size;
      bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;// Buffer is used as the copy source
      TAC_VULKAN_CALL( errors, vkCreateBuffer, mDevice, &bufferCreateInfo, nullptr, &cpuBuffer );
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements( mDevice, cpuBuffer, &memReqs );


    VkDeviceMemory cpuMemory;
    {
      VkMemoryPropertyFlags properties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | // <-- so we ( the cpu ) can copy our data here
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // <-- so our writes are visible to the GPU after unmapping

      VkMemoryAllocateInfo memAlloc = {};
      memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = GetMemory( memReqs.memoryTypeBits, properties, errors );
      TAC_HANDLE_ERROR( errors );
      TAC_VULKAN_CALL( errors, vkAllocateMemory, mDevice, &memAlloc, nullptr, &cpuMemory );
    }

    void* mappedData;
    TAC_VULKAN_CALL( errors, vkMapMemory, mDevice, cpuMemory, 0, size, 0, &mappedData );
    TacMemCpy( mappedData, data, ( int )size );
    vkUnmapMemory( mDevice, cpuMemory );

    TAC_VULKAN_CALL( errors, vkBindBufferMemory, mDevice, cpuBuffer, cpuMemory, 0 );


    VkBuffer gpuBuffer;
    {
      VkBufferCreateInfo bufferCreateInfo = {};
      bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferCreateInfo.size = size;
      bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
      TAC_VULKAN_CALL( errors, vkCreateBuffer, mDevice, &bufferCreateInfo, nullptr, &gpuBuffer );
    }

    vkGetBufferMemoryRequirements( mDevice, gpuBuffer, &memReqs );

    VkDeviceMemory gpuMemory;
    {
      VkMemoryAllocateInfo memAlloc = {};
      memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = GetMemory( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, errors );
      TAC_HANDLE_ERROR( errors );
      TAC_VULKAN_CALL( errors, vkAllocateMemory, mDevice, &memAlloc, nullptr, &gpuMemory );
    }
    TAC_VULKAN_CALL( errors, vkBindBufferMemory, mDevice, gpuBuffer, gpuMemory, 0 );

    VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = nullptr;

    VkCommandBuffer cmdBuffer;
    {
      VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
      cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      cmdBufAllocateInfo.commandPool = mCommandPool;
      cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      cmdBufAllocateInfo.commandBufferCount = 1;

      TAC_VULKAN_CALL( errors, vkAllocateCommandBuffers, mDevice, &cmdBufAllocateInfo, &cmdBuffer );
    }

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    TAC_VULKAN_CALL( errors, vkBeginCommandBuffer, cmdBuffer, &cmdBufInfo );

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer( cmdBuffer, cpuBuffer, gpuBuffer, 1, &copyRegion );


    // Flushing the command buffer will also submit it to the queue and uses a fence to ensure that all commands have been executed before returning
    FlushCommandBuffer( cmdBuffer, errors );

    // Destroy staging buffers, ( can only be done after copies have been submitted and executed )
    vkDestroyBuffer( mDevice, cpuBuffer, nullptr );
    vkFreeMemory( mDevice, cpuMemory, nullptr );

    auto indexBuffer = new TacVulkanIndexBuffer();
    *( TacIndexBufferData* )indexBuffer = indexBufferData;
    indexBuffer->mGpuBuffer = gpuBuffer;
    indexBuffer->mGpuMemory = gpuMemory;
    *outputIndexBuffer = indexBuffer;
  }

  void TacVulkanRenderer::AddBlendState( TacBlendState** outputBlendState, TacBlendStateData blendStateData, TacErrors& errors ) 
  {
    TacBlendConstants srcRGB = blendStateData.srcRGB;
    TacBlendConstants dstRGB = blendStateData.dstRGB;
    TacBlendMode blendRGB = blendStateData.blendRGB;
    TacBlendConstants srcA = blendStateData.srcA;
    TacBlendConstants dstA = blendStateData.dstA;
    TacBlendMode blendA = blendStateData.blendA;
    const TacString& debugName = blendStateData.mName;

    auto blendState = new TacVulkanBlendState();

    VkPipelineColorBlendAttachmentState blendAttachmentState = {};
    blendAttachmentState.colorWriteMask = 0xf;
    blendAttachmentState.blendEnable = VK_TRUE;
    blendState->blendAttachmentStates.push_back( blendAttachmentState );

    blendState->colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendState->colorBlendState.attachmentCount = blendState->blendAttachmentStates.size();
    blendState->colorBlendState.pAttachments = blendState->blendAttachmentStates.data();
    *outputBlendState = blendState;
  }

  void TacVulkanRenderer::AddRasterizerState( TacRasterizerState** outputRasterizerState, TacRasterizerStateData rasterizerStateData, TacErrors& errors ) 
  {
    TacFillMode fillMode = rasterizerStateData.fillMode;
    TacCullMode cullMode = rasterizerStateData.cullMode;
    bool frontCounterClockwise = rasterizerStateData.frontCounterClockwise;
    bool scissor = rasterizerStateData.scissor;
    bool multisample = rasterizerStateData.multisample;
    const TacString& debugName = rasterizerStateData.mName;

    TacUnusedParameter( scissor );
    TacUnusedParameter( multisample );

    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode = GetPolygonMode.at( fillMode );
    rasterizationState.cullMode = GetCullMode.at( cullMode );
    rasterizationState.frontFace = GetFrontFace( frontCounterClockwise );
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.lineWidth = 1.0f;

    auto vrasterizerState = new TacVulkanRasterizerState();
    vrasterizerState->mRasterizationState = rasterizationState;
    vrasterizerState->mName = debugName;
    *outputRasterizerState = vrasterizerState;
  }

  void TacVulkanRenderer::AddDepthState( TacDepthState** outputDepthState, TacDepthStateData depthStateData, TacErrors& errors ) 
  {
    bool depthTest = depthStateData.depthTest;
    bool depthWrite = depthStateData.depthWrite;
    TacDepthFunc depthFunc = depthStateData.depthFunc;
    const TacString& debugName = depthStateData.mName;

    VkStencilOpState stencilOpState = {};
    stencilOpState.failOp = VK_STENCIL_OP_KEEP;
    stencilOpState.passOp = VK_STENCIL_OP_KEEP;
    stencilOpState.compareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = TacVulkanBool( depthTest );
    depthStencilState.depthWriteEnable = TacVulkanBool( depthWrite );
    depthStencilState.depthCompareOp = TacVulkanCompareOp.at( depthFunc );
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = stencilOpState;
    depthStencilState.back = stencilOpState;

    auto depthState = new TacVulkanDepthState();
    depthState->mDepthStencilState = depthStencilState;
    depthState->mName = debugName;
    *outputDepthState = depthState;
  }

  void TacVulkanRenderer::CreateWindowContext( struct TacDesktopWindow* desktopWindow, TacErrors& errors ) 
  {
    uint32_t width = ( uint32_t )desktopWindow->mWidth;
    uint32_t height = ( uint32_t )desktopWindow->mHeight;


    TacVulkanGlobals* vg = TacVulkanGlobals::Instance();
    VkSurfaceKHR surface;
    vg->mCreateSurface( mShell, mInstance, desktopWindow, &surface, errors );
    TAC_HANDLE_ERROR( errors );

    VkSurfaceCapabilitiesKHR surface_capabilities;
    TAC_VULKAN_CALL( errors, vkGetPhysicalDeviceSurfaceCapabilitiesKHR, mPhysicalDevice, surface, &surface_capabilities );

    VkBool32 supported;
    TAC_VULKAN_CALL( errors, vkGetPhysicalDeviceSurfaceSupportKHR, mPhysicalDevice, mSelectedQueueFamilyIndex, surface, &supported );

    uint32_t formatCount;
    TAC_VULKAN_CALL( errors, vkGetPhysicalDeviceSurfaceFormatsKHR, mPhysicalDevice, surface, &formatCount, nullptr );
    if( !formatCount )
    {
      errors = "no surface formats!";
      return;
    }
    TacVector<VkSurfaceFormatKHR> surfaceFormats( formatCount );
    TAC_VULKAN_CALL( errors, vkGetPhysicalDeviceSurfaceFormatsKHR, mPhysicalDevice, surface, &formatCount, surfaceFormats.data() );

    VkImage depthStencilImage;
    {
      VkImageCreateInfo image = {};
      image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      image.pNext = NULL;
      image.imageType = VK_IMAGE_TYPE_2D;
      image.format = mDepthFormat;
      image.extent = { width, height, 1 };
      image.mipLevels = 1;
      image.arrayLayers = 1;
      image.samples = VK_SAMPLE_COUNT_1_BIT;
      image.tiling = VK_IMAGE_TILING_OPTIMAL;
      image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
      image.flags = 0;
      TAC_VULKAN_CALL( errors, vkCreateImage, mDevice, &image, nullptr, &depthStencilImage );
    }


    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements( mDevice, depthStencilImage, &memReqs );
    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;
    mem_alloc.allocationSize = memReqs.size;
    mem_alloc.memoryTypeIndex = GetMemory( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, errors );
    TAC_HANDLE_ERROR( errors );

    VkDeviceMemory depthStencilMemory = nullptr;
    TAC_VULKAN_CALL( errors, vkAllocateMemory, mDevice, &mem_alloc, nullptr, &depthStencilMemory );
    TAC_VULKAN_CALL( errors, vkBindImageMemory, mDevice, depthStencilImage, depthStencilMemory, 0 );

    VkImageView depthStencilImageView;
    {
      VkImageViewCreateInfo depthStencilView = {};
      depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      depthStencilView.pNext = NULL;
      depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      depthStencilView.format = mDepthFormat;
      depthStencilView.flags = 0;
      depthStencilView.subresourceRange = {};
      depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      depthStencilView.subresourceRange.baseMipLevel = 0;
      depthStencilView.subresourceRange.levelCount = 1;
      depthStencilView.subresourceRange.baseArrayLayer = 0;
      depthStencilView.subresourceRange.layerCount = 1;
      depthStencilView.image = depthStencilImage;
      TAC_VULKAN_CALL( errors, vkCreateImageView, mDevice, &depthStencilView, nullptr, &depthStencilImageView );
    }

    uint32_t desired_number_of_images = 2;
    VkSurfaceFormatKHR desired_format;
    //desired_format.format = VK_FORMAT_R8G8B8A8_UNORM;
    //desired_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    desired_format = surfaceFormats[ 0 ];



    VkExtent2D desired_extent;
    desired_extent.height = height;
    desired_extent.width = width;
    TacAssert( desired_extent.width && desired_extent.height );

    VkImageUsageFlags desired_usage = 0;
    // note: my gpu doesn't support depth stencil i guess?
    for( VkImageUsageFlagBits bits : { VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT } )
    {
      if( surface_capabilities.supportedUsageFlags & bits )
      {
        desired_usage |= bits;
      }
    }

    VkSurfaceTransformFlagBitsKHR desired_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

    VkPresentModeKHR desired_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    //VkSwapchainKHR old_swap_chain = mOldSwapChain;

    VkSwapchainCreateInfoKHR swap_chain_create_info = {};
    swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain_create_info.pNext = nullptr;
    swap_chain_create_info.flags = 0;
    swap_chain_create_info.surface = surface;
    swap_chain_create_info.minImageCount = desired_number_of_images;
    swap_chain_create_info.imageFormat = desired_format.format;
    swap_chain_create_info.imageColorSpace = desired_format.colorSpace;
    swap_chain_create_info.imageExtent = desired_extent;
    swap_chain_create_info.imageArrayLayers = 1;
    swap_chain_create_info.imageUsage = desired_usage;
    swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_chain_create_info.queueFamilyIndexCount = 0;
    swap_chain_create_info.pQueueFamilyIndices = nullptr;
    swap_chain_create_info.preTransform = desired_transform;
    swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_chain_create_info.presentMode = desired_present_mode;
    swap_chain_create_info.clipped = VK_TRUE;
    //swap_chain_create_info.oldSwapchain = old_swap_chain;

    VkSwapchainKHR SwapChain;
    TAC_VULKAN_CALL( errors, vkCreateSwapchainKHR, mDevice, &swap_chain_create_info, nullptr, &SwapChain );
    //if( old_swap_chain != VK_NULL_HANDLE )
    //{
    //  vkDestroySwapchainKHR( mDevice, old_swap_chain, nullptr );
    //}

    //mOldSwapChain = SwapChain;

    // I may need to call vkCreateImageView?



    uint32_t swapchainImageCount;
    TAC_VULKAN_CALL( errors, vkGetSwapchainImagesKHR, mDevice, SwapChain, &swapchainImageCount, nullptr );
    TacAssert( swapchainImageCount );
    TacVector< VkImage > images( swapchainImageCount );
    TAC_VULKAN_CALL( errors, vkGetSwapchainImagesKHR, mDevice, SwapChain, &swapchainImageCount, images.data() );


    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = mCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = swapchainImageCount;
    TacVector< VkCommandBuffer> commandBuffers( swapchainImageCount );

    TAC_VULKAN_CALL( errors, vkAllocateCommandBuffers, mDevice, &commandBufferAllocateInfo, commandBuffers.data() );


    VkRenderPass renderPass;
    {
      TacVector< VkAttachmentDescription > attachments;
      // add color attachment
      {
        VkAttachmentDescription attachment = {};
        attachment.format = desired_format.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments.push_back( attachment );
      }
      // add depth attachment
      {
        VkAttachmentDescription attachment = {};
        attachment.format = mDepthFormat;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back( attachment );
      }

      VkAttachmentReference colorReference = {};
      colorReference.attachment = 0;
      colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentReference depthReference = {};
      depthReference.attachment = 1;
      depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpassDescription = {};
      subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpassDescription.colorAttachmentCount = 1;
      subpassDescription.pColorAttachments = &colorReference;
      subpassDescription.pDepthStencilAttachment = &depthReference;
      subpassDescription.inputAttachmentCount = 0;
      subpassDescription.pInputAttachments = nullptr;
      subpassDescription.preserveAttachmentCount = 0;
      subpassDescription.pPreserveAttachments = nullptr;
      subpassDescription.pResolveAttachments = nullptr;

      // Subpass dependencies for layout transitions
      TacVector<VkSubpassDependency> dependencies;

      // Color dependency
      {
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies.push_back( dependency );
      }
      // Depth dependency
      {
        VkSubpassDependency dependency = {};

        dependency.srcSubpass = 0;
        dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies.push_back( dependency );
      }

      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = static_cast< uint32_t >( attachments.size() );
      renderPassInfo.pAttachments = attachments.data();
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpassDescription;
      renderPassInfo.dependencyCount = static_cast< uint32_t >( dependencies.size() );
      renderPassInfo.pDependencies = dependencies.data();

      TAC_VULKAN_CALL( errors, vkCreateRenderPass, mDevice, &renderPassInfo, nullptr, &renderPass );
    }


    TacVector< TacSwapChainBuffer > swapchainImages;

    for( int iSwapChainImage = 0; iSwapChainImage < ( int )swapchainImageCount; ++iSwapChainImage )
    {
      VkImage image = images[ iSwapChainImage ];

      VkImageView view;
      {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.format = desired_format.format;
        colorAttachmentView.components = {
          VK_COMPONENT_SWIZZLE_R,
          VK_COMPONENT_SWIZZLE_G,
          VK_COMPONENT_SWIZZLE_B,
          VK_COMPONENT_SWIZZLE_A
        };
        colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.image = image;
        TAC_VULKAN_CALL( errors, vkCreateImageView, mDevice, &colorAttachmentView, nullptr, &view );
      }

      // should this be per swap chain? per window? per application?
      VkFence fences;
      {
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        TAC_VULKAN_CALL( errors, vkCreateFence, mDevice, &fenceCreateInfo, nullptr, &fences );
      }

      VkFramebuffer frameBuffer;
      {
        TacVector< VkImageView > attachments = { view, depthStencilImageView };
        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.pNext = NULL;
        frameBufferCreateInfo.renderPass = renderPass;
        frameBufferCreateInfo.attachmentCount = attachments.size();
        frameBufferCreateInfo.pAttachments = attachments.data();
        frameBufferCreateInfo.width = width;
        frameBufferCreateInfo.height = height;
        frameBufferCreateInfo.layers = 1;
        TAC_VULKAN_CALL( errors, vkCreateFramebuffer, mDevice, &frameBufferCreateInfo, nullptr, &frameBuffer );
      }

      TacSwapChainBuffer buf;
      buf.mImage = image;
      buf.mImageView = view;
      buf.mCommandBuffer = commandBuffers[ iSwapChainImage ];
      buf.waitFence = fences;
      buf.mFrameBuffer = frameBuffer;
      swapchainImages.push_back( buf );
    }

    TacVulkanPerWindowData* perWindowData = new TacVulkanPerWindowData();
    perWindowData->mSwapChain = SwapChain;
    perWindowData->mSwapchainImages = swapchainImages;
    perWindowData->mDepthStencilImage = depthStencilImage;
    perWindowData->mDepthStencilImageView = depthStencilImageView;
    perWindowData->mDepthStencilMemory = depthStencilMemory;
    perWindowData->mColorFormat = desired_format.format;

    mPerWindowDatas.insert( perWindowData );
    desktopWindow->mRendererData = ( TacRendererWindowData* )perWindowData;

  }

  uint32_t TacVulkanRenderer::GetMemory( uint32_t typeBits, VkMemoryPropertyFlags properties, TacErrors& errors )
  {
    for( uint32_t i = 0; i < mDeviceMemoryProperties.memoryTypeCount; i++ )
    {
      if( ( typeBits & 1 ) == 1 )
      {
        if( ( mDeviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties ) == properties )
        {
          return i;
        }
      }
      typeBits >>= 1;
    }
    errors = "Vulkan get memory failed";
    return 0;
  }

  void TacVulkanRenderer::FlushCommandBuffer( VkCommandBuffer commandBuffer, TacErrors& errors )
  {
    TacAssert( commandBuffer != VK_NULL_HANDLE );

    TAC_VULKAN_CALL( errors, vkEndCommandBuffer, commandBuffer );

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // create fence to ensure that the command buffer has finished executing
    VkFence fence;
    {
      VkFenceCreateInfo fenceCreateInfo = {};
      fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceCreateInfo.flags = 0;
      TAC_VULKAN_CALL( errors, vkCreateFence, mDevice, &fenceCreateInfo, nullptr, &fence );
    }

    // Submit to the queue
    TAC_VULKAN_CALL( errors, vkQueueSubmit, mQueue, 1, &submitInfo, fence );


    uint64_t timeoutNanoseconds = 100000000000;

    // Wait for the fence to signal that command buffer has finished executing
    TAC_VULKAN_CALL( errors, vkWaitForFences, mDevice, 1, &fence, VK_TRUE, timeoutNanoseconds );

    vkDestroyFence( mDevice, fence, nullptr );
    vkFreeCommandBuffers( mDevice, mCommandPool, 1, &commandBuffer );
  }


TacVulkanGlobals::TacVulkanGlobals()
{
  mRequiredExtensions =
  {
    VK_KHR_SURFACE_EXTENSION_NAME
  };
}
TacVulkanGlobals* TacVulkanGlobals::Instance()
{
  static TacVulkanGlobals vg;
  return &vg;
}

void TacVulkanCallAux( TacErrors& errors, TacString functionName, VkResult res )
{
  errors.mMessage += functionName + " returned " + TacToString( res );
}

const static int includedVulkanRendererFactory = []()
{
  static struct TacVulkanRendererFactory : public TacRendererFactory
  {
    TacVulkanRendererFactory()
    {
      mRendererName = RendererNameVulkan;
    }
    void CreateRenderer( TacRenderer** renderer ) override
    {
      *renderer = new TacVulkanRenderer();
    }
  } vulkanFactory;
  TacRendererFactory::GetRegistry().push_back( &vulkanFactory );
  return 0;
}( );
*/
