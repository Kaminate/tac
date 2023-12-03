#pragma once

#include "src/shell/windows/renderer/tac_dx.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/windows/renderer/dxgi/tac_dxgi.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  struct DX12AppHelloWindow : public App
  {
    DX12AppHelloWindow( const Config& );

    void Init( Errors& ) override;
    void Update( Errors& ) override;
    void Uninit( Errors& ) override;

  private:

    // Helper functions for Init
    void CreateDesktopWindow();
    void DX12EnableDebug( Errors& );
    void DX12CreateDevice( Errors& );
    void DX12CreateInfoQueue( Errors& );
    void CreateCommandQueue( Errors& );
    void CreateRTVDescriptorHeap( Errors& );
    void CreateCommandAllocator( Errors& );
    void CreateCommandList( Errors& );
    void CreateFence( Errors& );

    // Helper functions for Update()
    void DX12CreateSwapChain( Errors& );

    void CreateRenderTargetViews( Errors& );
    void PopulateCommandList( Errors& );
    void ExecuteCommandLists();
    void SwapChainPresent( Errors& );
    void WaitForPreviousFrame( Errors& );

    static const int                            bufferCount = 2;
    bool                                        m_dbgLayerEnabled = false;

    DesktopWindowHandle                         hDesktopWindow;

    // ID3D12 objects
    Render::PCom< ID3D12Device >                m_device;

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


    Render::PCom< ID3D12DescriptorHeap >      m_rtvHeap;
    UINT                                      m_rtvDescriptorSize{};
    Render::PCom< ID3D12CommandQueue >        m_commandQueue;
    Render::PCom< ID3D12CommandAllocator >    m_commandAllocator;
    Render::PCom< ID3D12GraphicsCommandList > m_commandList;
    Render::PCom< ID3D12Resource >            m_renderTargets[ bufferCount ];
    Render::PCom< ID3D12Fence >               m_fence;
    Render::PCom< ID3D12PipelineState >       m_pipelineState;
    Render::PCom< ID3D12InfoQueue >           m_infoQueue;
    Render::PCom< ID3D12InfoQueue1 >          m_infoQueue1;

    // DXGI objects
    Render::PCom< IDXGISwapChain4 >           m_swapChain;


    // Frame timings
    UINT                                      m_frameIndex{};
    HANDLE                                    m_fenceEvent{};
    UINT64                                    m_fenceValue{};
  };
} // namespace Tac

