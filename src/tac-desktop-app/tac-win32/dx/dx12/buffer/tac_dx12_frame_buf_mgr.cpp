#include "tac_dx12_frame_buf_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-rhi/render3/tac_render_backend.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{
  void DX12SwapChainMgr::Init( Params params )
  {
    //mDevice = device;
    //mCpuDescriptorHeapRTV = cpuDescriptorHeapRTV;
    mTextureMgr = params.mTextureManager;
    mCommandQueue = params.mCommandQueue;
  }

  void   DX12SwapChainMgr::CreateSwapChain( SwapChainHandle h,
                                            SwapChainParams params,
                                            Errors& errors )
  {
    const void* nwh { params.mNWH };
    const v2i size { params.mSize };
    const int iHandle { h.GetIndex() };

    //ID3D12Device* device { mDevice };

    const DXGI_FORMAT fmt{ TexFmtToDxgiFormat( params.mColorFmt ) };
    
    const SwapChainCreateInfo scInfo
    {
      .mHwnd        { ( HWND )nwh },
      .mDevice      { ( IUnknown* )mCommandQueue->GetCommandQueue() },
      .mBufferCount { TAC_SWAP_CHAIN_BUF_COUNT },
      .mWidth       { size.x },
      .mHeight      { size.y },
      .mFmt         { fmt },
    };
    TAC_ASSERT( scInfo.mDevice );

    TAC_CALL( PCom< IDXGISwapChain4 > swapChain{ DXGICreateSwapChain( scInfo, errors ) } );
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    TAC_CALL( swapChain->GetDesc1( &swapChainDesc ) );

    DX12SwapChainImages swapChainImages;

    for( UINT iSwapChainBuf { 0 }; iSwapChainBuf < TAC_SWAP_CHAIN_BUF_COUNT; iSwapChainBuf++ )
    {
      //const int iRTVDescriptor { iHandle * TAC_SWAP_CHAIN_BUF_COUNT };
      //const DX12DescriptorHeapAllocation rtv { mCpuDescriptorHeapRTV->Allocate() };
      //const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle { rtv.GetCPUHandle() };

      PCom< ID3D12Resource > renderTarget;
      TAC_DX12_CALL( swapChain->GetBuffer(
        iSwapChainBuf,
        renderTarget.iid(),
        renderTarget.ppv() ) );
      DX12SetName( renderTarget,
                   "Render Target "
                   + Tac::ToString( iHandle )
                   + ", "
                   + Tac::ToString( iSwapChainBuf ) );

      const TextureHandle textureHandle { AllocTextureHandle() };
      TAC_CALL( mTextureMgr->CreateRenderTargetColor( textureHandle, renderTarget, errors ) );

      swapChainImages.push_back( textureHandle );
    }
    

    TextureHandle swapChainDepth;
    if( params.mDepthFmt != TexFmt::kUnknown )
    {
      swapChainDepth = AllocTextureHandle();

      const Image image
      {
        .mWidth   { params.mSize.x },
        .mHeight  { params.mSize.y },
        .mFormat2 { params.mDepthFmt },
      };

      const String name{ "zbuf " + Tac::ToString( iHandle ) };
      const CreateTextureParams depthParams
      {
        .mImage        { image },
        .mBinding      { Binding::DepthStencil },
        .mAccess       { Usage::Default },
        .mCpuAccess    { CPUAccess::None },
        .mOptionalName { name },
      };
      TAC_CALL( mTextureMgr->CreateTexture( swapChainDepth, depthParams, errors ) );
    }

    mSwapChains[ iHandle ] = DX12SwapChain
    {
      .mNWH             { nwh },
      .mSize            { size },
      .mSwapChain       { swapChain },
      .mSwapChainDesc   { swapChainDesc },
      .mSwapChainImages { swapChainImages },
      .mSwapChainDepth  { swapChainDepth },
      .mSwapChainParams { params },
    };
  }

  void   DX12SwapChainMgr::ResizeSwapChain( SwapChainHandle h, v2i size )
  {
    DX12SwapChain& frameBuf { mSwapChains[ h.GetIndex() ] };
    if( frameBuf.mSize == size )
      return;

    OS::OSDebugBreak();
  }

  SwapChainParams DX12SwapChainMgr::GetSwapChainParams( SwapChainHandle h )
  {
    return mSwapChains[ h.GetIndex() ].mSwapChainParams;
  }

  void   DX12SwapChainMgr::DestroySwapChain( SwapChainHandle h )
  {
    if( h.IsValid() )
      mSwapChains[ h.GetIndex() ] = {};
  }

  DX12SwapChain* DX12SwapChainMgr::FindSwapChain( SwapChainHandle h )
  {
    return h.IsValid() ? &mSwapChains[ h.GetIndex() ] : nullptr;
  }

  TextureHandle   DX12SwapChainMgr::GetSwapChainCurrentColor( SwapChainHandle h )
  {
    DX12SwapChain* swapChain { FindSwapChain( h ) };
    if( !swapChain )
      return {};

    const UINT iBackBuffer{ swapChain->mSwapChain->GetCurrentBackBufferIndex() };
    return swapChain->mSwapChainImages[ iBackBuffer ];
  }

  TextureHandle   DX12SwapChainMgr::GetSwapChainDepth( SwapChainHandle h )
  {
    DX12SwapChain* swapChain { FindSwapChain( h ) };
    return swapChain ? swapChain->mSwapChainDepth : TextureHandle{};
  }
} // namespace Tac::Render
