#include "tac_example_dx12_1_window.h" // self-inc

#include "src/shell/tac_desktop_app.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/system/tac_os.h"
#include "src/common/math/tac_math.h"
#include "src/shell/windows/tac_win32.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/core/tac_error_handling.h"
#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h"
#include "src/common/containers/tac_array.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"


#pragma comment( lib, "d3d12.lib" ) // D3D12...

namespace Tac
{


  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Init

  void DX12AppHelloWindow::CreateDesktopWindow()
  {
    const OS::Monitor monitor = OS::OSGetPrimaryMonitor();
    const int s = Min( monitor.mWidth, monitor.mHeight ) / 2;
    const DesktopAppCreateWindowParams desktopParams
    {
      .mName = "DX12 Window",
      .mX = ( monitor.mWidth - s ) / 2,
      .mY = ( monitor.mHeight - s ) / 2,
      .mWidth = s,
      .mHeight = s,
    };
    hDesktopWindow = CreateTrackedWindow( desktopParams );

    //hDesktopWindow = DesktopAppCreateWindow( desktopParams );
    DesktopAppResizeControls( hDesktopWindow );
    DesktopAppMoveControls( hDesktopWindow );
    QuitProgramOnWindowClose( hDesktopWindow );
  }

  void DX12AppHelloWindow::DX12EnableDebug( Errors& errors )
  {
    if( !IsDebugMode )
      return;

    Render::PCom<ID3D12Debug> dx12debug;
    TAC_DX12_CALL( D3D12GetDebugInterface, dx12debug.iid(), dx12debug.ppv() );

    auto dx12debug6 = dx12debug.QueryInterface< ID3D12Debug6>();
    auto dx12debug5 = dx12debug.QueryInterface< ID3D12Debug5>();
    auto dx12debug4 = dx12debug.QueryInterface< ID3D12Debug4>();
    auto dx12debug3 = dx12debug.QueryInterface< ID3D12Debug3>();
    auto dx12debug2 = dx12debug.QueryInterface< ID3D12Debug2>();
    auto dx12debug1 = dx12debug.QueryInterface< ID3D12Debug1>();

    // EnableDebugLayer must be called before the device is created
    TAC_ASSERT( ! m_device );
    dx12debug->EnableDebugLayer();
    m_dbgLayerEnabled = true;




  }

  void DX12AppHelloWindow::DX12CreateInfoQueue( Errors& )
  {
    if( !IsDebugMode )
      return;

    TAC_ASSERT( m_dbgLayerEnabled );

    m_infoQueue = m_device.QueryInterface<ID3D12InfoQueue>( );
    m_infoQueue1 = m_device.QueryInterface<ID3D12InfoQueue1>( );

    TAC_ASSERT(m_infoQueue);

    // Make the application debug break when bad things happen
    m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE );
    m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
    m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE );
  }

  void DX12AppHelloWindow::DX12CreateDevice( Errors& errors )
  {
    TAC_DX12_CALL( D3D12CreateDevice,
                   ( IDXGIAdapter* )DXGIGetBestAdapter(),
                   D3D_FEATURE_LEVEL_12_1,
                   m_device.iid(),
                   m_device.ppv() );
    Render::DX12SetName( ( ID3D12Device* )m_device, "Device" );

    auto device10 = m_device.QueryInterface<ID3D12Device10>();
  }

  void DX12AppHelloWindow::CreateCommandQueue( Errors& errors )
  {
    const D3D12_COMMAND_QUEUE_DESC queueDesc
    {
      // Specifies a command buffer that the GPU can execute.
      // A direct command list doesn't inherit any GPU state.
      // [ ] Q: 
      // [ ] A(?): ( As opposed to a bundle command list, which does )
      //
      // [ ] Q: What GPU state does a bundle command list inherit?
      // [ ] A: 
      //
      // [ ] Q: differnece between createcommandQueue and createCOmmandList?
      //        why does createCommandQUeue have a D3D12_COMMAND_LIST_TYPE_...?
      // [ ] A: 
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };


    TAC_DX12_CALL( m_device->CreateCommandQueue,
                   &queueDesc,
                   m_commandQueue.iid(),
                   m_commandQueue.ppv() );
    Render::DX12SetName( (ID3D12CommandQueue*)m_commandQueue, "Command Queue" );
  }

  void DX12AppHelloWindow::CreateRTVDescriptorHeap( Errors& errors )
  {
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors
    // Descriptors are the primary unit of binding for a single resource in D3D12.



    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps
    // A descriptor heap is a collection of contiguous allocations of descriptors,
    // one allocation for every descriptor.
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = bufferCount,
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap,
                   &desc,
                   m_rtvHeap.iid(),
                   m_rtvHeap.ppv() );
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
  }

  void DX12AppHelloWindow::CreateCommandAllocator( Errors& errors )
  {
    TAC_ASSERT( m_device );
    TAC_DX12_CALL( m_device->CreateCommandAllocator,
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   m_commandAllocator.iid(),
                   m_commandAllocator.ppv()  );
  }

  void DX12AppHelloWindow::CreateCommandList( Errors& errors )
  {
    // Create the command list.
    TAC_DX12_CALL( m_device->CreateCommandList,
                   0,
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   (ID3D12CommandAllocator*)m_commandAllocator,
                   nullptr,
                   m_commandList.iid(),
                   m_commandList.ppv() );

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    TAC_DX12_CALL( m_commandList->Close );
  }

  void DX12AppHelloWindow::CreateFence( Errors& errors )
  {
    // Create synchronization objects.
    TAC_DX12_CALL( m_device->CreateFence, 0, D3D12_FENCE_FLAG_NONE, m_fence.iid(), m_fence.ppv() );
    m_fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
    TAC_RAISE_ERROR_IF( !m_fenceEvent, Win32GetLastErrorString() );
  }

  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Update

  void DX12AppHelloWindow::DX12CreateSwapChain( Errors& errors )
  {
    const DesktopWindowState* state = GetDesktopWindowState( hDesktopWindow );
    const auto hwnd = ( HWND )state->mNativeWindowHandle;
    if( !hwnd )
      return;

    TAC_ASSERT( m_commandQueue );


    m_swapChain = TAC_CALL( DXGICreateSwapChain,
                            hwnd,
                            (ID3D12CommandQueue*)m_commandQueue, // swap chain can force flush the queue
                            bufferCount,
                            ( UINT )state->mWidth,
                            ( UINT )state->mHeight,
                            errors );

  }

  void DX12AppHelloWindow::CreateRenderTargetViews( Errors& errors )
  {
    TAC_ASSERT( m_swapChain );
    TAC_ASSERT( m_device );


    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    // Create a RTV for each frame.
    for( UINT i = 0; i < bufferCount; i++ )
    {
      Render::PCom< ID3D12Resource >& renderTarget = m_renderTargets[ i ];
      TAC_DX12_CALL( m_swapChain->GetBuffer,
                     i,
                     renderTarget.iid(),
                     renderTarget.ppv() );
      m_device->CreateRenderTargetView( ( ID3D12Resource* )renderTarget, nullptr, rtvHandle );

      rtvHandle.ptr += m_rtvDescriptorSize;
    }


  }

  void DX12AppHelloWindow::PopulateCommandList( Errors& errors )
  {
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    TAC_DX12_CALL( m_commandAllocator->Reset );

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    TAC_DX12_CALL( m_commandList->Reset,
                   ( ID3D12CommandAllocator* )m_commandAllocator,
                   ( ID3D12PipelineState* )m_pipelineState );

    Render::PCom< ID3D12Resource >& resource = m_renderTargets[ m_frameIndex ];
    TAC_ASSERT( resource );


    const Array rtBarriers =
    {
      D3D12_RESOURCE_BARRIER
      {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = D3D12_RESOURCE_TRANSITION_BARRIER
        {
          .pResource = (ID3D12Resource*)resource,
          .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
          .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
          .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
        },
      }
    };

    // Indicate that the back buffer will be used as a render target.
    const UINT rtN = ( UINT )rtBarriers.size();
    const D3D12_RESOURCE_BARRIER* rts = rtBarriers.data();
    m_commandList->ResourceBarrier( rtN, rts );


    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle
    {
      m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_frameIndex * m_rtvDescriptorSize
    };


    const double speed = 3;
    const auto t = ( float )Sin( ShellGetElapsedSeconds() * speed ) * 0.5f + 0.5f;

    // Record commands.
    const float clearColor[] = { t, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );

    const Array presentBarriers =
    {
      D3D12_RESOURCE_BARRIER
      {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = D3D12_RESOURCE_TRANSITION_BARRIER
        {
          .pResource = (ID3D12Resource*)resource,
          .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
          .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
          .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
        },
      }
    };

    // Indicate that the back buffer will now be used to present.
    m_commandList->ResourceBarrier( ( UINT )presentBarriers.size(), presentBarriers.data() );

    TAC_DX12_CALL( m_commandList->Close );
  }

  void DX12AppHelloWindow::ExecuteCommandLists()
  {
    const Array lists = { ( ID3D12CommandList* )( ID3D12GraphicsCommandList* )m_commandList };
    const auto n = ( UINT )lists.size();
    const auto p = ( ID3D12CommandList** )lists.data();
    m_commandQueue->ExecuteCommandLists( n, p );
  }

  void DX12AppHelloWindow::SwapChainPresent( Errors& errors )
  {
    // Present the frame.
    TAC_DX12_CALL( m_swapChain->Present, 1, 0 );
  }

  void DX12AppHelloWindow::WaitForPreviousFrame( Errors& errors )
  {
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    TAC_DX12_CALL( m_commandQueue->Signal, (ID3D12Fence*)m_fence, fence );

    m_fenceValue++;

    // Wait until the previous frame is finished.
    if( m_fence->GetCompletedValue() < fence )
    {
      TAC_DX12_CALL( m_fence->SetEventOnCompletion, fence, m_fenceEvent );
      WaitForSingleObject( m_fenceEvent, INFINITE );
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
  }


  // -----------------------------------------------------------------------------------------------

  DX12AppHelloWindow::DX12AppHelloWindow( const Config& cfg ) : App( cfg ) {}

  void DX12AppHelloWindow::Init( Errors& errors )
  {
    CreateDesktopWindow();

    TAC_CALL( DXGIInit, errors );
    TAC_CALL( DX12EnableDebug, errors );
    TAC_CALL( DX12CreateDevice, errors );
    TAC_CALL( DX12CreateInfoQueue, errors );
    TAC_CALL( CreateCommandQueue, errors );
    TAC_CALL( CreateRTVDescriptorHeap, errors );
    TAC_CALL( CreateCommandAllocator, errors );
    TAC_CALL( CreateCommandList, errors );
    TAC_CALL( CreateFence, errors );
  }

  void DX12AppHelloWindow::Update( Errors& errors )
  {
    if( !GetDesktopWindowNativeHandle( hDesktopWindow ) )
      return;

    if( !m_swapChain )
    {
      TAC_CALL( DX12CreateSwapChain, errors );
      TAC_CALL( CreateRenderTargetViews, errors );
    }

    // Record all the commands we need to render the scene into the command list.
    TAC_CALL( PopulateCommandList, errors );

    ExecuteCommandLists();

    TAC_CALL( SwapChainPresent, errors );

    TAC_CALL( WaitForPreviousFrame, errors );

    ++asdf;
  }

  void DX12AppHelloWindow::Uninit( Errors& errors )
  {
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    TAC_CALL( WaitForPreviousFrame, errors );

    CloseHandle( m_fenceEvent );

    DXGIUninit();
  }

  App* App::Create()
  {
    const App::Config config
    {
      .mName = "DX12 Hello Window",
      .mDisableRenderer = true,
    };
    return TAC_NEW DX12AppHelloWindow( config );
  };


} // namespace Tac

