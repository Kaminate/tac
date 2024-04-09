#include "tac_dx12_frame_buf_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{
  void DX12FrameBufferMgr::Init( ID3D12Device* device,
                                 DX12CommandQueue* commandQueue,
                                 DX12DescriptorHeap* cpuDescriptorHeapRTV )
  {
    mDevice = device;
    mCpuDescriptorHeapRTV = cpuDescriptorHeapRTV;
    mCommandQueue = commandQueue;
  }

  void   DX12FrameBufferMgr::CreateFB( FBHandle h, FrameBufferParams params, Errors& errors )
  {
    const void* nwh = params.mNWH;
    const v2i size = params.mSize;
    const int iHandle = h.GetIndex();

    ID3D12Device* device = mDevice;

    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
    switch( params.mColorFmt )
    {
    case kD24S8: fmt = DXGI_FORMAT_D24_UNORM_S8_UINT; break;
    case kRGBA16F: fmt = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
    default: TAC_ASSERT_INVALID_CASE( params.mColorFmt ); break;
    };
    
    const SwapChainCreateInfo scInfo
    {
      .mHwnd = ( HWND )nwh,
      .mDevice = ( IUnknown* )mCommandQueue->GetCommandQueue(),
      .mBufferCount = TAC_SWAP_CHAIN_BUF_COUNT,
      .mWidth = size.x,
      .mHeight = size.y,
      .mFmt = fmt,
    };
    TAC_ASSERT( scInfo.mDevice );

    PCom< IDXGISwapChain4 > swapChain = TAC_CALL( DXGICreateSwapChain( scInfo, errors ) );
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    TAC_CALL( swapChain->GetDesc1( &swapChainDesc ) );

    DX12SwapChainImages swapChainImages;

    for( UINT iSwapChainBuf = 0; iSwapChainBuf < TAC_SWAP_CHAIN_BUF_COUNT; iSwapChainBuf++ )
    {
      const int iRTVDescriptor = iHandle * TAC_SWAP_CHAIN_BUF_COUNT;

      DX12DescriptorHeapAllocation rtv = mCpuDescriptorHeapRTV->Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtv.GetCPUHandle();

      PCom< ID3D12Resource > renderTarget;
      TAC_DX12_CALL( swapChain->GetBuffer(
        iSwapChainBuf,
        renderTarget.iid(),
        renderTarget.ppv() ) );

      device->CreateRenderTargetView( ( ID3D12Resource* )renderTarget, nullptr, rtvHandle );

      DX12SetName( renderTarget, "Render Target " + Tac::ToString( iHandle ) );

      swapChainImages[ iSwapChainBuf ] = DX12SwapChainImage
      {
        .mResource = renderTarget,
        .mDesc = renderTarget->GetDesc(),

        // the render target resource is created in a state that is ready to be displayed on screen
        .mState = D3D12_RESOURCE_STATE_PRESENT,
        .mRTV = rtv,
      };
    }

    mFrameBufs[ iHandle ] = DX12FrameBuf
    {
      .mNWH = nwh,
      .mSize = size,
      .mSwapChain = swapChain,
      .mSwapChainDesc = swapChainDesc,
      .mSwapChainImages = swapChainImages,
      .mFmt = params.mColorFmt,
    };
  }

  void   DX12FrameBufferMgr::ResizeFB( FBHandle h, v2i size )
  {

    DX12FrameBuf& frameBuf = mFrameBufs[ h.GetIndex() ];
    if( frameBuf.mSize == size )
      return;

    OS::OSDebugBreak();
  }

  TexFmt DX12FrameBufferMgr::GetFBFmt( FBHandle h )
  {
    return mFrameBufs[ h.GetIndex() ].mFmt;
  }


  void   DX12FrameBufferMgr::DestroyFB( FBHandle h )
  {
    if( h.IsValid() )
      mFrameBufs[ h.GetIndex() ] = {};
  }

  DX12FrameBuf* DX12FrameBufferMgr::FindFB( FBHandle h )
  {
    return h.IsValid() ? &mFrameBufs[ h.GetIndex() ] : nullptr;
  }
} // namespace Tac::Render
