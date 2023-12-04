#pragma once

#include "src/shell/windows/renderer/tac_dx.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/windows/renderer/dxgi/tac_dxgi.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  struct Win32Event
  {
    void Init(Errors&);
    ~Win32Event();
    void clear();

    void operator = ( Win32Event&& other );

    explicit operator HANDLE() const;

    HANDLE mEvent{};
  };

  struct DX12AppHelloWindow : public App
  {
    DX12AppHelloWindow( const Config& );

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
    void CreateFence( Errors& );

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

    static const int                            bufferCount = 2;
    bool                                        m_dbgLayerEnabled = false;

    DesktopWindowHandle                         hDesktopWindow;

    // ---------------------------------------------------------------------------------------------

    // ID3D12 objects

    Render::PCom< ID3D12Device5 >              m_device;

    Render::PCom< ID3D12DescriptorHeap >      m_rtvHeap;
    UINT                                      m_rtvDescriptorSize{};
    D3D12_CPU_DESCRIPTOR_HANDLE               m_rtvHeapStart;

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
    Render::PCom< ID3D12CommandQueue >         m_commandQueue;
    Render::PCom< ID3D12CommandAllocator >     m_commandAllocator;
    Render::PCom< ID3D12GraphicsCommandList4 > m_commandList;
    Render::PCom< ID3D12Resource >             m_renderTargets[ bufferCount ];
    D3D12_RESOURCE_STATES                      m_renderTargetStates[ bufferCount ];

    // A fence is used to synchronize the CPU with the GPU (see Multi-engine synchronization).
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
    Render::PCom< ID3D12Fence1 >               m_fence;
    Render::PCom< ID3D12PipelineState >        m_pipelineState;
    Render::PCom< ID3D12InfoQueue >            m_infoQueue;

    // ---------------------------------------------------------------------------------------------

    // DXGI objects

    Render::PCom< IDXGISwapChain4 >            m_swapChain;
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

