#include "tac_dx12_tutorial_1_window.h" // self-inc
#include "tac_dx12_tutorial.h"

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-win32/tac_win32.h"


#pragma comment( lib, "d3d12.lib" ) // D3D12...

namespace Tac
{
  using namespace Render;

  // A graphics root signature defines what resources are bound to the graphics pipeline.
  // A pipeline state object maintains the state of all currently set shaders as well as certain fixed function state objects (such as the input assembler, tesselator, rasterizer and output merger).

  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Init

  void DX12AppHelloWindow::EnableDebug( Errors& errors )
  {
    if constexpr( kIsDebugMode )
    {

      PCom<ID3D12Debug> dx12debug;
      TAC_DX12_CALL( D3D12GetDebugInterface( dx12debug.iid(), dx12debug.ppv() ) );

      auto dx12debug5{ dx12debug.QueryInterface< ID3D12Debug5 >() };
      auto dx12debug4{ dx12debug.QueryInterface< ID3D12Debug4 >() };
      auto dx12debug3{ dx12debug.QueryInterface< ID3D12Debug3 >() };
      auto dx12debug2{ dx12debug.QueryInterface< ID3D12Debug2 >() };
      auto dx12debug1{ dx12debug.QueryInterface< ID3D12Debug1 >() };

      // EnableDebugLayer must be called before the device is created
      TAC_ASSERT( !m_device );
      dx12debug->EnableDebugLayer();
      m_dbgLayerEnabled = true;
    }
  }

  void DX12AppHelloWindow::CreateInfoQueue( Errors& )
  {
    if constexpr( kIsDebugMode )
    {

      TAC_ASSERT( m_dbgLayerEnabled );

      m_infoQueue = m_device.QueryInterface<ID3D12InfoQueue>();

      TAC_ASSERT( m_infoQueue );

      // Make the application debug break when bad things happen
      m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE );
      m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
      m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE );
    }
  }

  void DX12AppHelloWindow::CreateDevice( Errors& errors )
  {
    
    auto adapter { ( IDXGIAdapter* )DXGIGetBestAdapter() };
    PCom< ID3D12Device > device;
    TAC_DX12_CALL( D3D12CreateDevice(
                   adapter,
                   D3D_FEATURE_LEVEL_12_1,
                   device.iid(),
                   device.ppv() ) );
    m_device = device.QueryInterface<ID3D12Device5>();
    DX12SetName( m_device, "Device" );
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
      .Type { D3D12_COMMAND_LIST_TYPE_DIRECT },
    };


    TAC_DX12_CALL( m_device->CreateCommandQueue(
                   &queueDesc,
                   m_commandQueue.iid(),
                   m_commandQueue.ppv() ) );
    DX12SetName( m_commandQueue, "Command Queue" );
  }

  void DX12AppHelloWindow::CreateRTVDescriptorHeap( Errors& errors )
  {
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors
    // Descriptors are the primary unit of binding for a single resource in D3D12.



    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps
    // A descriptor heap is a collection of contiguous allocations of descriptors,
    // one allocation for every descriptor.
    const D3D12_DESCRIPTOR_HEAP_DESC desc
    {
      .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_RTV },
      .NumDescriptors { bufferCount },
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap(
                   &desc,
                   m_rtvHeap.iid(),
                   m_rtvHeap.ppv() ) );
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    m_rtvHeapStart = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloWindow::CreateCommandAllocator( Errors& errors )
  {
    // a command allocator manages storage for cmd lists and bundles
    TAC_ASSERT( m_device );
    TAC_DX12_CALL( m_device->CreateCommandAllocator(
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   m_commandAllocator.iid(),
                   m_commandAllocator.ppv()  ) );
  }

  void DX12AppHelloWindow::CreateCommandList( Errors& errors )
  {
    // Create the command list (CreateCommandList1 creates it in a closed state).
    PCom< ID3D12CommandList > commandList;
    TAC_DX12_CALL( m_device->CreateCommandList1(
                   0,
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   D3D12_COMMAND_LIST_FLAG_NONE,
                   m_commandList.iid(),
                   m_commandList.ppv() ) );

    commandList.QueryInterface( m_commandList );
    TAC_ASSERT(m_commandList);
  }

  void DX12AppHelloWindow::CreateFence( Errors& errors )
  {
    // Create synchronization objects.

    const UINT64 initialVal {};

    PCom< ID3D12Fence > fence;
    TAC_DX12_CALL( m_device->CreateFence(
                   initialVal,
                   D3D12_FENCE_FLAG_NONE,
                   fence.iid(),
                   fence.ppv() ) );

    fence.QueryInterface(m_fence);

    m_fenceValue = 1;

    TAC_CALL( m_fenceEvent.Init( errors ) );
  }

  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Update

  void DX12AppHelloWindow::DX12CreateSwapChain( Errors& errors )
  {
    if( m_swapChainValid )
      return;

    auto hwnd { ( HWND )AppWindowApi::GetNativeWindowHandle( hDesktopWindow ) };
    if( !hwnd )
      return;

    const v2i size { AppWindowApi::GetSize( hDesktopWindow ) };

    TAC_ASSERT( m_commandQueue );

    const DXGISwapChainWrapper::Params scInfo
    {
      .mHwnd        { hwnd },
      .mDevice      { ( IUnknown* )m_commandQueue }, // swap chain can force flush the queue
      .mBufferCount { bufferCount },
      .mWidth       { size.x },
      .mHeight      { size.y },
      .mFmt         { RTVFormat },
    };
    TAC_CALL( m_swapChain.Init( scInfo, errors ) );
    TAC_CALL( m_swapChain->GetDesc1( &m_swapChainDesc ) );
    m_swapChainValid  = true;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloWindow::GetRenderTargetDescriptorHandle( int i ) const
  {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle { m_rtvHeapStart };
    rtvHandle.ptr += i * m_rtvDescriptorSize;
    return rtvHandle;
  }

  void DX12AppHelloWindow::CreateRenderTargetViews( Errors& errors )
  {
    TAC_ASSERT( m_swapChain );
    TAC_ASSERT( m_device );

    // Create a RTV for each frame.
    for( UINT i{}; i < bufferCount; i++ )
    {
      const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{ GetRenderTargetDescriptorHandle( i ) };
      PCom< ID3D12Resource >& renderTarget{ m_renderTargets[ i ] };
      TAC_DX12_CALL( m_swapChain->GetBuffer( i, renderTarget.iid(), renderTarget.ppv() ) );
      m_device->CreateRenderTargetView( ( ID3D12Resource* )renderTarget, nullptr, rtvHandle );

      // the render target resource is created in a state that is ready to be displayed on screen
      m_renderTargetStates[i] = D3D12_RESOURCE_STATE_PRESENT;
    }
  }

  void DX12AppHelloWindow::TransitionRenderTarget( const int iRT,
                                                   const D3D12_RESOURCE_STATES targetState )
  {
    ID3D12Resource* rtResource { (ID3D12Resource*)m_renderTargets[ iRT ] };
    TAC_ASSERT(rtResource );

    const D3D12_RESOURCE_STATES before { m_renderTargetStates[ iRT ] };
    TAC_ASSERT( before != targetState );

    const D3D12_RESOURCE_TRANSITION_BARRIER Transition
    {
      .pResource   { rtResource },
      .Subresource { D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES },
      .StateBefore { before },
      .StateAfter  { targetState },
    };

    const D3D12_RESOURCE_BARRIER barrier
    {
      .Type       { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION },
      .Transition { Transition },
    };

    m_renderTargetStates[ iRT ] = targetState;

    ResourceBarrier( barrier );
  }

  void DX12AppHelloWindow::ResourceBarrier( const D3D12_RESOURCE_BARRIER& barrier )
  {
    // Resource barriers are used to manage resource transitions.

    // ID3D12CommandList::ResourceBarrier
    // - Notifies the driver that it needs to synchronize multiple accesses to resources.
    //
    const Array barriers { barrier };
    const UINT rtN { ( UINT )barriers.size() };
    const D3D12_RESOURCE_BARRIER* rts { barriers.data() };
    m_commandList->ResourceBarrier( rtN, rts );
  }

  void DX12AppHelloWindow::PopulateCommandList( Errors& errors )
  {
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    //
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset
    // ID3D12CommandAllocator::Reset
    //   Indicates to re-use the memory that is associated with the command allocator.
    //   From this call to Reset, the runtime and driver determine that the GPU is no longer
    //   executing any command lists that have recorded commands with the command allocator.
    TAC_DX12_CALL( m_commandAllocator->Reset() );

    // However( when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    //
    // ID3D12GraphicsCommandList::Reset
    //   you can re-use command list tracking structures without any allocations
    //   you can call Reset while the command list is still being executed
    //   you can submit a cmd list, reset it, and reuse the allocated memory for another cmd list
    TAC_DX12_CALL( m_commandList->Reset(
                   ( ID3D12CommandAllocator* )m_commandAllocator,
                   ( ID3D12PipelineState* )m_pipelineState ) );

    // Indicate that the back buffer will be used as a render target.
    TransitionRenderTarget( m_frameIndex, D3D12_RESOURCE_STATE_RENDER_TARGET );

    ClearRenderTargetView();

    // Indicate that the back buffer will now be used to present.
    //
    // When a back buffer is presented, it must be in the D3D12_RESOURCE_STATE_PRESENT state.
    // If IDXGISwapChain1::Present1 is called on a resource which is not in the PRESENT state,
    // a debug layer warning will be emitted.
    TransitionRenderTarget( m_frameIndex, D3D12_RESOURCE_STATE_PRESENT );

    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( m_commandList->Close() );
  }

  void DX12AppHelloWindow::ClearRenderTargetView()
  {
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle { GetRenderTargetDescriptorHandle( m_frameIndex ) };

    const double speed { 3 };
    const auto t { ( float )Sin( GameTimer::GetElapsedTime() * speed ) * 0.5f + 0.5f };

    // Record commands.
    const float clearColor[]  { t, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );
  }

  void DX12AppHelloWindow::ExecuteCommandLists()
  {
    const Array lists
    {
      ( ID3D12CommandList* )( ID3D12GraphicsCommandList* )m_commandList
    };

    // Submits an array of command lists for execution.
    m_commandQueue->ExecuteCommandLists( ( UINT )lists.size(), lists.data() );
  }

  void DX12AppHelloWindow::SwapChainPresent( Errors& errors )
  {
    // Present the frame.

    // [x] Q: What is the frame?
    //        Is it the current frame?
    //        The last frame?
    //     A: From experimentation, you can consider the 'frame' to refer to
    //        the render target indexed by the swap chain's current backbuffer index.
    //        (m_renderTargets[m_swapChain->GetCurrentBackBufferIndex())
    //        
    //        This render target is in D3D12_RESOURCE_STATE_PRESENT,
    //        And after calling swap chain IDXGISwapChain::Present(), the next call to 
    //        IDXGISwapChain::GetCurrentBackBufferIndex() will be 1 higher
    //
    //        I think the swap chain flushes the command queue before rendering,
    //        so the frame being presented is the one that we just called ExecuteCommandLists() on

    TAC_CALL( DXGICheckSwapEffect( m_swapChainDesc.SwapEffect, errors ) );

    const DXGI_PRESENT_PARAMETERS params{};

    // For the flip model (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL), values are:
    //   0   - Cancel the remaining time on the previously presented frame
    //         and discard this frame if a newer frame is queued.
    //   1-4 - Synchronize presentation for at least n vertical blanks.
    const UINT SyncInterval { 1 };
    const UINT PresentFlags {};

    // I think this technically adds a frame onto the present queue
    TAC_DX12_CALL( m_swapChain->Present1( SyncInterval, PresentFlags, &params ) );
  }

  void DX12AppHelloWindow::WaitForPreviousFrame( Errors& errors )
  {
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // ID3D12CommandQueue::Signal
    // - Updates a fence to a specified value.
    // ^ I think this adds a signal command to the queue, which basically delay sets the fence
    //   to a value
    // ^ A command queue is a not a command list, so this maybe adds a signal to be fired
    //   in between execution of command lists?

    // Signal and increment the fence value.

    // (first call)
    //   m_fence was initialized with value 0,
    //   m_fenceValue was initialized to 1
    //
    //   so we signal m_fence(0) with m_fenceValue(1)
    //   increment m_fenceValue(2)
    //

    const UINT64 signalValue { m_fenceValue };

    // Use this method to set a fence value from the GPU side
    // [ ] Q: ^ ???
    TAC_DX12_CALL( m_commandQueue->Signal( (ID3D12Fence*)m_fence, signalValue ) );

    m_fenceValue++;

    // Experimentally, m_fence->GetCompletedValue() doesn't hit the signalled value until

    // ID3D12Fence::GetCompletedValue
    // - Gets the current value of the fence.
    //
    // Wait until the previous frame is finished.

    // I think this if statement is used because the alternative
    // would be while( m_fence->GetCompletedValue() != fence ) { TAC_NO_OP; }
    const UINT64 curValue { m_fence->GetCompletedValue() };
    if( curValue < signalValue )
    {
      // m_fenceEvent is only ever used in this scope 

      // ID3D12Fence::SetEventOnCompletion
      // - Specifies an event that's raised when the fence reaches a certain value.
      //
      // the event will be 'complete' when it reaches the specified value.
      // This value is set by the cmdqueue::Signal
      TAC_DX12_CALL( m_fence->SetEventOnCompletion( signalValue, ( HANDLE )m_fenceEvent ) );
      WaitForSingleObject( (HANDLE)m_fenceEvent, INFINITE );
    }

    // I think this would be the same if it were called after IDXGISwapChain::Present instead
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
  }


  // -----------------------------------------------------------------------------------------------

  DX12AppHelloWindow::DX12AppHelloWindow( const Config& cfg ) : App( cfg ) {}

  void DX12AppHelloWindow::Init( Errors& errors )
  {
    AppWindowApi::SetSwapChainAutoCreate( false );
    TAC_CALL( hDesktopWindow = DX12ExampleCreateWindow("DX12 Window", errors ) );

    TAC_CALL( DXGIInit( errors ) );
    TAC_CALL( EnableDebug( errors ) );
    TAC_CALL( CreateDevice( errors ) );
    TAC_CALL( CreateInfoQueue( errors ) );
    TAC_CALL( CreateCommandQueue( errors ) );
    TAC_CALL( CreateRTVDescriptorHeap( errors ) );
    TAC_CALL( CreateCommandAllocator( errors ) );
    TAC_CALL( CreateCommandList( errors ) );
    TAC_CALL( CreateFence( errors ) );
  }

  void DX12AppHelloWindow::Update( Errors& )
  {
  }

  void DX12AppHelloWindow::Render( RenderParams , Errors& errors )
  {

    if( !AppWindowApi::IsShown( hDesktopWindow ) )
      return;

    if( !m_swapChain )
    {
      TAC_CALL( DX12CreateSwapChain( errors ) );
      TAC_CALL( CreateRenderTargetViews( errors ) );
    }

    // Record all the commands we need to render the scene into the command list.
    TAC_CALL( PopulateCommandList( errors ) );

    ExecuteCommandLists();

    TAC_CALL( SwapChainPresent( errors ) );
    TAC_CALL( WaitForPreviousFrame( errors ) );
  }

  void DX12AppHelloWindow::Uninit( Errors& errors )
  {
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    TAC_CALL( WaitForPreviousFrame( errors ));

    DXGIUninit();
  }

  App* App::Create()
  {
    const App::Config config
    {
      .mName            { "DX12 Hello Window" },
      .mDisableRenderer { true },
    };
    return TAC_NEW DX12AppHelloWindow( config );
  };


} // namespace Tac

