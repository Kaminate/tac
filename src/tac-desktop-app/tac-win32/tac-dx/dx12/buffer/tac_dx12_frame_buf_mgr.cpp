#include "tac_dx12_frame_buf_mgr.h" // self-inc
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-rhi/render3/tac_render_backend.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{

  auto DX12SwapChainMgr::CreateSwapChain( SwapChainParams params, Errors& errors ) -> SwapChainHandle
  {
    DX12CommandQueue* commandQueue{ &DX12Renderer::sRenderer.mCommandQueue };
    const void* nwh { params.mNWH };
    const v2i size { params.mSize };
    const DXGI_FORMAT fmt{ DXGIFormatFromTexFmt( params.mColorFmt ) };
    
    const DXGISwapChainWrapper::Params dxgiSwapChainParams
    {
      .mHwnd        { ( HWND )nwh },
      .mDevice      { ( IUnknown* )commandQueue->GetCommandQueue() },
      .mBufferCount { TAC_SWAP_CHAIN_BUF_COUNT },
      .mWidth       { size.x },
      .mHeight      { size.y },
      .mFmt         { fmt },
    };
    TAC_ASSERT( dxgiSwapChainParams.mDevice );

    DXGISwapChainWrapper swapChain;
    TAC_CALL_RET( swapChain.Init( dxgiSwapChainParams, errors ) );

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    TAC_DX12_CALL_RET( swapChain->GetDesc1( &swapChainDesc ) );

    TAC_CALL_RET( const DX12SwapChainImages swapChainImages{
      CreateColorTextures( swapChain, params, errors ) } );

    TAC_CALL_RET( const TextureHandle swapChainDepth{
      CreateDepthTexture( size, params, errors ) } );

    const SwapChainHandle h{ AllocSwapChainHandle() };
    const int iHandle { h.GetIndex() };

    NameRTTextures( h, swapChainImages, swapChainDepth );

    mSwapChains[ iHandle ] = DX12SwapChain
    {
      .mNWH               { nwh },
      .mSize              { size },
      .mDXGISwapChain     { swapChain },
      .mDXGISwapChainDesc { swapChainDesc },
      .mRTColors          { swapChainImages },
      .mRTDepth           { swapChainDepth },
      .mSwapChainParams   { params },
    };
    return h;
  }

  auto DX12SwapChainMgr::CreateColorTextures( DXGISwapChainWrapper swapChain,
                                              SwapChainParams params,
                                              Errors& errors ) -> DX12SwapChainImages
  {
    DX12TextureMgr* textureMgr{ &DX12Renderer::sRenderer.mTexMgr };
    DX12SwapChainImages swapChainImages;
    for( UINT iSwapChainBuf{}; iSwapChainBuf < TAC_SWAP_CHAIN_BUF_COUNT; iSwapChainBuf++ )
    {
      PCom< ID3D12Resource > rsc;
      TAC_DX12_CALL_RET( swapChain->GetBuffer( iSwapChainBuf, rsc.iid(), rsc.ppv() ) );
      const TextureHandle textureHandle { AllocTextureHandle() };
      TAC_CALL_RET( textureMgr->CreateRenderTargetColor(
        textureHandle, rsc, params, iSwapChainBuf, errors ) );
      swapChainImages.push_back( textureHandle );
    }

    return swapChainImages;
  }

  auto DX12SwapChainMgr::CreateDepthTexture( v2i size,
                                             SwapChainParams params,
                                             Errors& errors ) -> TextureHandle
  {
    const TexFmt fmt{ params.mDepthFmt };

    DX12TextureMgr* textureMgr{ &DX12Renderer::sRenderer.mTexMgr };
    if( fmt == TexFmt::kUnknown )
      return {};

    const Image image
    {
      .mWidth  { size.x },
      .mHeight { size.y },
      .mFormat { fmt },
    };

    const String name{ params.mName + " depth" };
    const CreateTextureParams depthParams
    {
      .mImage        { image },
      .mMipCount     { 1 },
      .mBinding      { Binding::DepthStencil },
      .mUsage        { Usage::Default },
      .mCpuAccess    { CPUAccess::None },
      .mOptionalName { name },
    };

    return textureMgr->CreateTexture( depthParams, errors );
  }

  void DX12SwapChainMgr::ResizeSwapChain( const SwapChainHandle h,
                                            const v2i size,
                                            Errors& errors )
  {
    DX12TextureMgr* textureMgr{ &DX12Renderer::sRenderer.mTexMgr };
    DX12CommandQueue* commandQueue{ &DX12Renderer::sRenderer.mCommandQueue };
    DX12SwapChain& swapChain { mSwapChains[ h.GetIndex() ] };
    if( swapChain.mSize == size )
      return;

    DX12SwapChainImages     mSwapChainImages; // Color backbuffer
    TextureHandle           mSwapChainDepth; // Depth backbuffer

    // Before resizing a swap chain, all resources must be cleared. Thus we
    // 1) wait for the command queue to be idle
    // 2) free the textures
    // 3) resize the swap chain


    /* shit...

    CORRUPTION: An ID3D12Resource object (0x000002B94AB20B30:'rtcolor0[2]) is referenced by GPU
    operations in-flight on Command Queue (0x000002B947845A50:'Command Queue').
    It is not safe to final-release objects that may have GPU operations pending.
    This can result in application instability.
    [ EXECUTION ERROR #921: OBJECT_DELETED_WHILE_STILL_IN_USE]
    */
    TAC_CALL( commandQueue->WaitForIdle( errors ) );

    for( const TextureHandle swapChainColor : swapChain.mRTColors )
      textureMgr->DestroyTexture( swapChainColor );

    textureMgr->DestroyTexture( swapChain.mRTDepth );

    TAC_CALL( swapChain.mDXGISwapChain.Resize( size, errors ) );

    swapChain.mSize = size;

    TAC_DX12_CALL( swapChain.mDXGISwapChain->GetDesc1( &swapChain.mDXGISwapChainDesc ) );

    TAC_CALL( const TextureHandle swapChainDepth{
      CreateDepthTexture( size, swapChain.mSwapChainParams, errors ) } );

    TAC_CALL( DX12SwapChainImages swapChainImages{
      CreateColorTextures( swapChain.mDXGISwapChain, swapChain.mSwapChainParams, errors ) } );

    NameRTTextures( h, swapChainImages, swapChainDepth );

    swapChain.mRTDepth = swapChainDepth;
    swapChain.mRTColors = swapChainImages;
  }

  void DX12SwapChainMgr::NameRTTextures( SwapChainHandle h,
                                         DX12SwapChainImages swapChainImages,
                                         TextureHandle swapChainDepth )
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12TextureMgr* mTextureMgr{ &renderer.mTexMgr };
    //DX12CommandQueue* mCommandQueue{ &renderer.mCommandQueue };
    const int iHandle{ h.GetIndex() };
    for( int i{}; i < swapChainImages.size(); ++i )
    {
      TextureHandle textureHandle{ swapChainImages[ i ] };
      const String name{ "rtcolor" + Tac::ToString( iHandle ) + "[" + Tac::ToString( i ) + "]" };
      mTextureMgr->SetName( textureHandle, name );
    }

    const String rtDepthName{ "rtdepth" + ToString( iHandle ) };
    mTextureMgr->SetName( swapChainDepth, rtDepthName );
  }

  auto DX12SwapChainMgr::GetSwapChainParams( SwapChainHandle h ) -> SwapChainParams
  {
    return mSwapChains[ h.GetIndex() ].mSwapChainParams;
  }

  void DX12SwapChainMgr::DestroySwapChain( SwapChainHandle h )
  {
    DX12TextureMgr* mTextureMgr{ &DX12Renderer::sRenderer.mTexMgr };
    //DX12CommandQueue* mCommandQueue{ &DX12Renderer::sRenderer.mCommandQueue };
    if( h.IsValid() )
    {
      FreeHandle( h );
      DX12SwapChain& swapChain { mSwapChains[ h.GetIndex() ] };

      mTextureMgr->DestroyTexture( swapChain.mRTDepth );
      for( TextureHandle rtColor : swapChain.mRTColors )
        mTextureMgr->DestroyTexture( rtColor );

      swapChain = {};
    }
  }

  auto DX12SwapChainMgr::FindSwapChain( SwapChainHandle h ) -> DX12SwapChain*
  {
    return h.IsValid() ? &mSwapChains[ h.GetIndex() ] : nullptr;
  }

  auto DX12SwapChainMgr::GetSwapChainCurrentColor( SwapChainHandle h ) -> TextureHandle
  {
    DX12SwapChain* swapChain { FindSwapChain( h ) };
    if( !swapChain )
      return {};

    const UINT iBackBuffer{ swapChain->mDXGISwapChain->GetCurrentBackBufferIndex() };
    return swapChain->mRTColors[ iBackBuffer ];
  }


  auto DX12SwapChainMgr::GetSwapChainDepth( SwapChainHandle h ) -> TextureHandle
  {
    DX12SwapChain* swapChain { FindSwapChain( h ) };
    return swapChain ? swapChain->mRTDepth : TextureHandle{};
  }
} // namespace Tac::Render
