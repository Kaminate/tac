#pragma once

#include "src/shell/windows/renderer/tac_dx.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/windows/renderer/dxgi/tac_dxgi.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  using namespace Render;

  struct Win32Event
  {
    void Init(Errors&);
    ~Win32Event();
    void clear();

    void operator = ( Win32Event&& other );

    explicit operator HANDLE() const;

    HANDLE mEvent{};
  };

  inline const int kViewportCount = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
  inline const int kScissorRectCount = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;

  struct DX12AppHelloTriangle : public App
  {
    DX12AppHelloTriangle( const Config& );

    void Init( Errors& ) override;
    void Update( Errors& ) override;
    void Uninit( Errors& ) override;

  private:

    // Helper functions for Init
    void CreateDesktopWindow();
    void EnableDebug( Errors& );
    void CreateDevice( Errors& );
    void CreateInfoQueue( Errors& );
    void CreateCommandQueue( Errors& );
    void CreateRTVDescriptorHeap( Errors& );
    void CreateCommandAllocator( Errors& );
    void CreateCommandList( Errors& );
    void CreateVertexBuffer( Errors& );
    void CreateFence( Errors& );
    void CreateRootSignature( Errors&);
    void CreatePipelineState( Errors&);

    // Helper functions for Update()
    void DX12CreateSwapChain( Errors& );
    void CreateRenderTargetViews( Errors& );
    void ClearRenderTargetView();
    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetDescriptorHandle(int) const;
    void PopulateCommandList( Errors& );
    void ExecuteCommandLists();
    void ResourceBarrier( const D3D12_RESOURCE_BARRIER& );
    void TransitionRenderTarget( int, D3D12_RESOURCE_STATES );
    void SwapChainPresent( Errors& );
    void WaitForPreviousFrame( Errors& );

    // ---------------------------------------------------------------------------------------------

    static const int                           bufferCount = 2;

    DesktopWindowHandle                        hDesktopWindow;

    // ---------------------------------------------------------------------------------------------

    // ID3D12 objects

    PCom< ID3D12Device5 >              m_device;

    PCom< ID3D12Debug3 >               m_debug;
    bool                                       m_debugLayerEnabled = false;
    PCom< ID3D12DescriptorHeap >       m_rtvHeap;
    UINT                                       m_rtvDescriptorSize{};
    D3D12_CPU_DESCRIPTOR_HANDLE                m_rtvHeapStart;

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
    PCom< ID3D12GraphicsCommandList4 > m_commandList;
    PCom< ID3D12Resource >             m_renderTargets[ bufferCount ];
    D3D12_RESOURCE_STATES                      m_renderTargetStates[ bufferCount ];

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

    PCom<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

    FixedVector< D3D12_VIEWPORT, kViewportCount > m_viewports;
    FixedVector< D3D12_RECT, kScissorRectCount > m_scissorRects;

    // ---------------------------------------------------------------------------------------------

    // DXGI objects

    PCom< IDXGISwapChain4 >            m_swapChain;
    DXGI_SWAP_CHAIN_DESC1                      m_swapChainDesc;
 
    // ---------------------------------------------------------------------------------------------

    // Frame timings

    // Index of the render target that
    // 1. our commands will be drawing onto
    // 2. our swap chain will present to the monitor
    UINT                                      m_frameIndex{};
    Win32Event                                m_fenceEvent;

    // UINT64 is big enough to run at 1000 fps for 500 million years
    UINT64                                    m_fenceValue{};
  };
} // namespace Tac

