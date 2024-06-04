#pragma once

#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-dx/dxgi/tac_dxgi.h"

#include "tac-std-lib/containers/tac_ring_vector.h"

#include "tac_dx12_tutorial_command_queue.h"
#include "tac_dx12_tutorial_command_allocator_pool.h"
#include "tac_dx12_tutorial_context_manager.h"
#include "tac_dx12_tutorial_gpu_upload_allocator.h"


#include <d3d12.h> // D3D12...

namespace Tac
{
  using namespace Render;

  // maximum number of frames submitted to the gpu at one time
  const int MAX_GPU_FRAME_COUNT = 2;

  // number of textures in the swap chain
  const int SWAP_CHAIN_BUFFER_COUNT = 3;

  using Viewports = FixedVector<
    D3D12_VIEWPORT,
    D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE >;

  using ScissorRects = FixedVector<
    D3D12_RECT,
    D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE >;

  struct DX12AppHelloFrameBuf : public App
  {
    enum SRVIndexes
    {
      TriangleVertexBuffer,
      TriangleTexture,
      Count,
    };

    struct State : public IState
    {
      float mTranslateX = 0;
    };


    DX12AppHelloFrameBuf( const Config& );

    void     Init( InitParams, Errors& ) override;
    void     Update( UpdateParams, Errors& ) override;
    void     Uninit( Errors& ) override;
    void     Render( RenderParams, Errors& ) override;
    IState*  GetGameState() override;

  private:

    // Helper functions for Init
    void PreSwapChainInit(Errors&);

    void CreateRTVDescriptorHeap( Errors& );
    void CreateSamplerDescriptorHeap( Errors& );
    void CreateSRVDescriptorHeap( Errors& );
    void CreateBufferSRV( Errors& );
    void CreateSampler( Errors& );
    void CreateTexture( Errors& );
    void CreateCommandAllocatorBundle( Errors& );
    void CreateCommandListBundle( Errors& );
    void CreateBuffer( Errors& );
    void CreateRootSignature( Errors& );
    void CreatePipelineState( Errors& );
    void InitDescriptorSizes();
    void RecordBundle();

    void RenderBegin( Errors& );
    void RenderEnd( Errors& );

    // Helper functions for Update()
    void DX12CreateSwapChain( const SysWindowApi*, Errors& );
    void CreateRenderTargetViews( Errors& );
    void ClearRenderTargetView( ID3D12GraphicsCommandList* );
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCpuDescHandle( int ) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGpuDescHandle( int ) const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpuDescHandle( int ) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpuDescHandle( int ) const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetSamplerCpuDescHandle( int ) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerGpuDescHandle( int ) const;

    D3D12_CPU_DESCRIPTOR_HANDLE OffsetCpuDescHandle( D3D12_CPU_DESCRIPTOR_HANDLE,
                                                     D3D12_DESCRIPTOR_HEAP_TYPE,
                                                     int ) const;
    D3D12_GPU_DESCRIPTOR_HANDLE OffsetGpuDescHandle( D3D12_GPU_DESCRIPTOR_HANDLE,
                                                     D3D12_DESCRIPTOR_HEAP_TYPE,
                                                     int ) const;
    void PopulateCommandList( DX12ExampleContextScope&, float translateX, float translateY, float scale, Errors& );
    void ResourceBarrier( ID3D12GraphicsCommandList*, const D3D12_RESOURCE_BARRIER& );

    struct TransitionParams
    {
       ID3D12Resource*        mResource;
       D3D12_RESOURCE_STATES* mCurrentState;
       D3D12_RESOURCE_STATES  mTargetState;
    };
    void TransitionResource( ID3D12GraphicsCommandList*, TransitionParams );

    void TransitionRenderTarget( ID3D12GraphicsCommandList*, int, D3D12_RESOURCE_STATES );
    void SwapChainPresent( Errors& );

    // ---------------------------------------------------------------------------------------------

    WindowHandle                hDesktopWindow{};

    // ---------------------------------------------------------------------------------------------

    // ID3D12 objects

    
    PCom< ID3D12Device >               m_device0;
    PCom< ID3D12Device5 >              m_device;

    PCom< ID3D12Debug3 >               m_debug;
    bool                               m_debugLayerEnabled = false;
    PCom<ID3D12DebugDevice>            m_debugDevice0;
    PCom<ID3D12DebugDevice2>           m_debugDevice;

    // samplers
    PCom< ID3D12DescriptorHeap >       m_samplerHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE        m_samplerCpuHeapStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE        m_samplerGpuHeapStart{};

    // rtvs
    PCom< ID3D12DescriptorHeap >       m_rtvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE        m_rtvCpuHeapStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE        m_rtvGpuHeapStart{};

    // srvs
    PCom< ID3D12DescriptorHeap >       m_srvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE        m_srvCpuHeapStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE        m_srvGpuHeapStart{};

    UINT                               m_descriptorSizes[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ]{};

    PCom< ID3D12CommandAllocator >     m_commandAllocatorBundle;
    PCom< ID3D12GraphicsCommandList4 > m_commandListBundle;
    PCom< ID3D12Resource >             m_renderTargets[ SWAP_CHAIN_BUFFER_COUNT ];
    D3D12_RESOURCE_STATES              m_renderTargetStates[ SWAP_CHAIN_BUFFER_COUNT ]{};
    D3D12_RESOURCE_DESC                m_renderTargetDescs[ SWAP_CHAIN_BUFFER_COUNT ]{};
    bool                               m_renderTargetInitialized = false;

    PCom< ID3D12InfoQueue >            m_infoQueue;

    // A root signature defines what resources are bound to the graphics pipeline.
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signatures
    //
    // I think you're supposed to think of the rootsignature as a function signature of the entire
    // graphics pipeline. A function signature is composed of function parameters/arguments,
    // or in the dx12 case, rootparameters/rootarguments (SRV descriptors, CBV descriptors, etc)
    PCom< ID3D12RootSignature >        m_rootSignature;

    // A pipeline state object maintains the state of all currently set shaders as well as certain
    // fixed function state objects
    // (such as the input assembler, tesselator, rasterizer and output merger).
    PCom< ID3D12PipelineState >        mPipelineState;

    PCom<ID3D12Resource>               m_texture;
    D3D12_RESOURCE_DESC                m_textureDesc{};
    D3D12_RESOURCE_STATES              m_textureResourceStates{};

    PCom<ID3D12Resource>               m_vertexBufferUploadHeap;
    PCom<ID3D12Resource>               m_vertexBuffer;
    //D3D12_VERTEX_BUFFER_VIEW           m_vertexBufferView;
    UINT                               m_vertexBufferByteCount{};

    D3D12_VIEWPORT                     m_viewport{};
    D3D12_RECT                         m_scissorRect{};

    Viewports                          m_viewports{};
    ScissorRects                       m_scissorRects{};

    // ---------------------------------------------------------------------------------------------

    // DXGI objects

    DXGISwapChainWrapper               m_swapChain;
    DXGI_SWAP_CHAIN_DESC1              m_swapChainDesc{};
    bool                               m_swapChainValid{};
 
    // ---------------------------------------------------------------------------------------------

    // Frame timings

    // Index of the render target that
    // 1. our commands will be drawing onto
    // 2. our swap chain will present to the monitor
    int                                m_backbufferIndex{};

    // total number of frames sent to the gpu
    u64                                mSentGPUFrameCount = 0;

    // index of the next gpu frame to be in-flight ( see also MAX_GPU_FRAME_COUNT )
    u64                                m_gpuFlightFrameIndex = 0;


    DX12ExampleCommandQueue                   mCommandQueue;
    DX12ExampleCommandAllocatorPool           mCommandAllocatorPool;
    DX12ExampleContextManager                 mContextManager;
    DX12ExampleGPUUploadPageManager               mUploadPageManager;

    State                              mState;

    FenceSignal                        mFenceValues[ MAX_GPU_FRAME_COUNT ]{};
  };
} // namespace Tac

