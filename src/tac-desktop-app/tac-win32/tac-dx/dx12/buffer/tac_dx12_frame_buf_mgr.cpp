#include "tac_dx12_frame_buf_mgr.h" // self-inc
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
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
    
    const DXGISwapChainWrapper::Params dxgiSwapChainParams
    {
      .mHwnd        { ( HWND )nwh },
      .mDevice      { ( IUnknown* )mCommandQueue->GetCommandQueue() },
      .mBufferCount { TAC_SWAP_CHAIN_BUF_COUNT },
      .mWidth       { size.x },
      .mHeight      { size.y },
      .mFmt         { fmt },
    };
    TAC_ASSERT( dxgiSwapChainParams.mDevice );

    DXGISwapChainWrapper swapChain;
    TAC_CALL( swapChain.Init( dxgiSwapChainParams, errors ) );

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    TAC_DX12_CALL( swapChain->GetDesc1( &swapChainDesc ) );

    TAC_CALL( DX12SwapChainImages swapChainImages{ CreateColorTextures( h, swapChain, errors ) } );

    TAC_CALL( const TextureHandle swapChainDepth{
      CreateDepthTexture( h, size, params.mDepthFmt, errors ) } );

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

  DX12SwapChainImages DX12SwapChainMgr::CreateColorTextures( SwapChainHandle h,
                                                             DXGISwapChainWrapper swapChain,
                                                             Errors& errors )
  {
    DX12SwapChainImages swapChainImages;

    for( UINT iSwapChainBuf {}; iSwapChainBuf < TAC_SWAP_CHAIN_BUF_COUNT; iSwapChainBuf++ )
    {
      //const int iRTVDescriptor { iHandle * TAC_SWAP_CHAIN_BUF_COUNT };
      //const DX12DescriptorHeapAllocation rtv { mCpuDescriptorHeapRTV->Allocate() };
      //const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle { rtv.GetCPUHandle() };

      PCom< ID3D12Resource > renderTarget;
      TAC_DX12_CALL_RET( {}, swapChain->GetBuffer(
        iSwapChainBuf,
        renderTarget.iid(),
        renderTarget.ppv() ) );
      ID3D12Resource* resource{ renderTarget.Get() };
      DX12SetName( resource,
                   "rtcolor"
                   + Tac::ToString( h.GetIndex() )
                   + "["
                   + Tac::ToString( iSwapChainBuf )
                   + "]" );

      const TextureHandle textureHandle { AllocTextureHandle() };
      TAC_CALL_RET( {}, mTextureMgr->CreateRenderTargetColor(
        textureHandle, renderTarget, errors ) );

      swapChainImages.push_back( textureHandle );
    }

    return swapChainImages;
  }

  TextureHandle   DX12SwapChainMgr::CreateDepthTexture( SwapChainHandle h,
                                                        v2i size,
                                                        TexFmt fmt,
                                                        Errors& errors )
  {
    if( fmt == TexFmt::kUnknown )
      return {};

    const TextureHandle swapChainDepth { AllocTextureHandle() };
    const Image image
    {
      .mWidth  { size.x },
      .mHeight { size.y },
      .mFormat { fmt },
    };

    const String name{ "rtdepth" + ToString( h.GetIndex() ) };
    const CreateTextureParams depthParams
    {
      .mImage        { image },
      .mMipCount     { 1 },
      .mBinding      { Binding::DepthStencil },
      .mUsage        { Usage::Default },
      .mCpuAccess    { CPUAccess::None },
      .mOptionalName { name },
    };
    TAC_CALL_RET( {}, mTextureMgr->CreateTexture( swapChainDepth, depthParams, errors ) );
    return swapChainDepth;
  }

  void   DX12SwapChainMgr::ResizeSwapChain( const SwapChainHandle h,
                                            const v2i size,
                                            Errors& errors )
  {
    DX12SwapChain& frameBuf { mSwapChains[ h.GetIndex() ] };
    if( frameBuf.mSize == size )
      return;

    DX12SwapChainImages     mSwapChainImages; // Color backbuffer
    TextureHandle           mSwapChainDepth; // Depth backbuffer

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    // Before resizing a swap chain, all resources must be cleared
    for( const TextureHandle swapChainColor : frameBuf.mSwapChainImages )
    {
      //mTextureMgr->DestroyTexture( swapChainColor );
      // ... texturemgr  doesnt destroy the index
      renderDevice->DestroyTexture( swapChainColor );
    }

    //mTextureMgr->DestroyTexture( frameBuf.mSwapChainDepth );
    // ... texturemgr  doesnt destroy the index
    renderDevice->DestroyTexture( frameBuf.mSwapChainDepth );

    TAC_CALL( mCommandQueue->WaitForIdle( errors ) );


    TAC_CALL( frameBuf.mSwapChain.Resize( size, errors ) );
    frameBuf.mSize = size;

    TAC_DX12_CALL( frameBuf.mSwapChain->GetDesc1( &frameBuf.mSwapChainDesc ) );

    TAC_CALL( const TextureHandle swapChainDepth{
      CreateDepthTexture( h, size, frameBuf.mSwapChainParams.mDepthFmt, errors ) } );

    TAC_CALL( DX12SwapChainImages swapChainImages{
      CreateColorTextures( h, frameBuf.mSwapChain, errors ) } );

    frameBuf.mSwapChainDepth = swapChainDepth;
    frameBuf.mSwapChainImages = swapChainImages;
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
