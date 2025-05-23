#pragma once

#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-win32/event/tac_win32_event.h"
#include "tac-dx/dxgi/tac_dxgi.h"
#include "tac-engine-core/window/tac_window_handle.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  using namespace Render;

  using Viewports = FixedVector<
    D3D12_VIEWPORT,
    D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE >;

  using ScissorRects = FixedVector<
    D3D12_RECT,
    D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE >;

  struct DX12AppHelloTexture : public App
  {
    enum SRVIndexes
    {
      TriangleVertexBuffer,
      TriangleTexture,
      Count,
    };

    DX12AppHelloTexture( const Config& );

    void Init( Errors& ) override;
    void Update( Errors& ) override;
    void Render( RenderParams, Errors& ) override;
    void Uninit( Errors& ) override;

  private:

    // Helper functions for Init
    void PreSwapChainInit(Errors&);

    void EnableDebug( Errors& );

    void CreateDevice( Errors& );
    void CreateInfoQueue( Errors& );
    void CreateCommandQueue( Errors& );
    void CreateRTVDescriptorHeap( Errors& );
    void CreateSamplerDescriptorHeap( Errors& );
    void CreateSRVDescriptorHeap( Errors& );
    void CreateBufferSRV( Errors& );
    void CreateSampler( Errors& );
    void CreateTexture( Errors& );
    void CreateCommandAllocator( Errors& );
    void CreateCommandList( Errors& );
    void CreateBuffer( Errors& );
    void CreateFence( Errors& );
    void CreateRootSignature( Errors& );
    void CreatePipelineState( Errors& );
    void InitDescriptorSizes();
    

    // Helper functions for Update()
    void DX12CreateSwapChain( Errors& );
    void CreateRenderTargetViews( Errors& );
    void ClearRenderTargetView();
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
    void PopulateCommandList( Errors& );
    void ExecuteCommandLists();
    void ResourceBarrier( const D3D12_RESOURCE_BARRIER& );

    struct TransitionParams
    {
       ID3D12Resource*        mResource;
       D3D12_RESOURCE_STATES* mCurrentState;
       D3D12_RESOURCE_STATES  mTargetState;
    };
    void TransitionResource( TransitionParams );

    void TransitionRenderTarget( int, D3D12_RESOURCE_STATES );
    void SwapChainPresent( Errors& );
    void WaitForPreviousFrame( Errors& );

    // ---------------------------------------------------------------------------------------------

    static const int                   bufferCount { 2 };

    WindowHandle                       hDesktopWindow{};

    // ---------------------------------------------------------------------------------------------

    // ID3D12 objects

    PCom< ID3D12Device5 >              mDevice;

    PCom< ID3D12Debug3 >               m_debug;
    bool                               m_debugLayerEnabled = false;
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

    // A ID3D12CommandQueue provides methods for
    // - submitting command lists,
    // - synchronizing command list execution,
    // - instrumenting the command queue,
    // - etc
    //
    // Some examples:
    // - ID3D12CommandQueue::ExecuteCommandLists
    // - ID3D12CommandQueue::GetClockCalibration
    // - ID3D12CommandQueue::GetTimestampFrequency
    // - ID3D12CommandQueue::Signal
    // - ID3D12CommandQueue::Wait
    // 
    // Together, CommandLists/CommandQueues replace the ID3D11DeviceContext (?)
    //
    // tldr: A command queue can submit command lists
    PCom< ID3D12CommandQueue >         m_commandQueue;
    PCom< ID3D12CommandAllocator >     m_commandAllocator;
    PCom< ID3D12CommandAllocator >     m_commandAllocatorBundle;
    PCom< ID3D12GraphicsCommandList4 > m_commandList;
    PCom< ID3D12GraphicsCommandList4 > m_commandListBundle;
    PCom< ID3D12Resource >             m_renderTargets[ bufferCount ];
    D3D12_RESOURCE_STATES              m_renderTargetStates[ bufferCount ];
    D3D12_RESOURCE_DESC                m_renderTargetDescs[ bufferCount ];
    bool                               m_renderTargetInitialized {};

    // A fence is used to synchronize the CPU with the GPU (see Multi-engine synchronization).
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
    PCom< ID3D12Fence1 >               m_fence;
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
    bool                               m_vertexBufferCopied {};

    D3D12_VIEWPORT                     m_viewport{};
    D3D12_RECT                         m_scissorRect{};

    Viewports                          m_viewports;
    ScissorRects                       m_scissorRects;

    // ---------------------------------------------------------------------------------------------

    // DXGI objects

    DXGISwapChainWrapper               m_swapChain;
    DXGI_SWAP_CHAIN_DESC1              m_swapChainDesc{};
    DXGI_FORMAT                        RTVFormat { DXGI_FORMAT_R16G16B16A16_FLOAT };
 
    // ---------------------------------------------------------------------------------------------

    // Frame timings

    // Index of the render target that
    // 1. our commands will be drawing onto
    // 2. our swap chain will present to the monitor
    UINT                               m_frameIndex{};
    Win32Event                         m_fenceEvent;

    // UINT64 is big enough to run at 1000 fps for 500 million years
    UINT64                             m_fenceValue{};
  };
} // namespace Tac

