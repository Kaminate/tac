#include "tac_renderer_dx12_ver3.h" // self-inc

//#include "tac-rhi/render3/tac_render_api.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h" // OS::DebugBreak
#endif
//#include "tac-rhi/render/tac_render.h"
//#include "tac-rhi/render/tac_render.h"
//#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
#include "tac-win32/dx/dxgi/tac_dxgi.h" // DXGICreateSwapChain

#pragma comment( lib, "d3d12.lib" ) // D3D12...

namespace Tac::Render
{

#if 0
  // -----------------------------------------------------------------------------------------------

  // DX12CommandList

  void DX12CommandList::Draw()
  {
    ID3D12GraphicsCommandList* cmdList = mContext.GetCommandList();
    const UINT vtxCountPerInstance = 0;
    const UINT instanceCount = 0;
    const UINT startVertexLocation = 0;
    const UINT startIndexLocation = 0;
    cmdList->DrawInstanced( vtxCountPerInstance,
                            instanceCount,
                            startVertexLocation,
                            startIndexLocation );
  }

  // -----------------------------------------------------------------------------------------------

  // DX12DynBuf

  void DX12DynBuf::SetName( StringView name )
  {
    DX12SetName( mResource, name );
  }

  // -----------------------------------------------------------------------------------------------

  // DX12Backend
#endif

  const int TAC_MAX_FB_COUNT = 100;

  void DX12Backend::Init( Errors& errors )
  {
    TAC_CALL( DXGIInit( errors ) );

    TAC_CALL( mDebugLayer.Init( errors ) );

    TAC_CALL( mDevice.Init( mDebugLayer, errors ) );

    ID3D12Device* device = mDevice.GetID3D12Device();

    TAC_CALL( mInfoQueue.Init( mDebugLayer, device, errors ) );

    TAC_CALL( mRTVDescriptorHeap.InitRTV( TAC_MAX_FB_COUNT * TAC_SWAP_CHAIN_BUF_COUNT, device, errors ) );

    TAC_CALL( mShaderProgramMgr.Init( device, errors ) );

    //const int maxGPUFrameCount = RenderApi::GetMaxGPUFrameCount();
    /*
    const int maxGPUFrameCount = Render::GetMaxGPUFrameCount();
    TAC_ASSERT( maxGPUFrameCount );
    mFenceValues.resize( maxGPUFrameCount );

    TAC_CALL( DXGIInit( errors ) );

    TAC_CALL( debugLayer.Init( errors ) );

    TAC_CALL( mDevice.Init( debugLayer, errors ) );
    ID3D12Device* device = mDevice.GetID3D12Device();

    TAC_CALL( infoQueue.Init( debugLayer, device, errors ) );

    TAC_CALL( mCommandQueue.Create( device, errors ) );

    TAC_CALL( mSRVDescriptorHeap.InitSRV( 100, device, errors ) );
    TAC_CALL( mSamplerDescriptorHeap.InitSampler( 100, device, errors ) );

    mCommandAllocatorPool.Init( device, &mCommandQueue );
    mContextManager.Init( &mCommandAllocatorPool,
                          &mCommandQueue,
                          &mUploadPageManager,
                          device );

    mUploadPageManager.Init( device, &mCommandQueue );

    mSamplers.Init( device, &mSamplerDescriptorHeap );
    */
  }

#if 0

  SmartPtr< ICommandList > DX12Backend::GetCommandList( ContextHandle handle, Errors& errors )
  {
    const int i = handle.GetHandleIndex();
    if( !( i < mContexts.size() ) )
    {
      mContexts.resize( i + 1 );
    }

    DX12Context context = mContextManager.GetContextNoScope( errors );
    mContexts[ i ] = context;

    DX12CommandList* dx12CmdList = TAC_NEW DX12CommandList;
    dx12CmdList->mContext = context;

    return SmartPtr< ICommandList >{ dx12CmdList };
  }

  void DX12Backend::CreateDynamicBuffer2( const DynBufCreateParams& params, Errors& errors )
  {
    const D3D12_HEAP_PROPERTIES HeapProps
    {
      .Type = D3D12_HEAP_TYPE_UPLOAD,
      .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
      .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
      .CreationNodeMask = 1,
      .VisibleNodeMask = 1,
    };

    const D3D12_RESOURCE_DESC ResourceDesc
    {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
      .Alignment = 0,
      .Width = ( UINT64 )params.mByteCount,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc = DXGI_SAMPLE_DESC
      {
        .Count = 1,
        .Quality = 0
      },
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    const D3D12_RESOURCE_STATES DefaultUsage{ D3D12_RESOURCE_STATE_GENERIC_READ };

    ID3D12Device* device = mDevice.GetID3D12Device();

    PCom< ID3D12Resource > buffer;
    TAC_DX12_CALL( device->CreateCommittedResource(
      &HeapProps,
      D3D12_HEAP_FLAG_NONE,
      &ResourceDesc,
      DefaultUsage,
      nullptr,
      buffer.iid(),
      buffer.ppv() ) );

    DX12SetName( buffer, params.mStackFrame );

    void* cpuAddr;

    TAC_DX12_CALL( buffer->Map(
      0, // subrsc idx
      nullptr, // nullptr indicates the whole subrsc may be read by cpu
      &cpuAddr ) );

    const int i = params.mHandle.GetHandleIndex();
    const int n = mDynBufs.size();
    if( !( i < n ) )
      mDynBufs.resize( i + 1 );

    mDynBufs[ i ] = DX12DynBuf
    {
      .mResource = buffer,
      .mMappedCPUAddr = cpuAddr,
    };
  }

  void DX12Backend::UpdateDynamicBuffer2( const DynBufUpdateParams& params )
  {
    const void* srcBytes = params.mUpdateMemory->GetBytes();
    const int srcByteCount = params.mUpdateMemory->GetByteCount();

    const int iBuf = params.mHandle.GetHandleIndex();
    DX12DynBuf& dynBuf = mDynBufs[ iBuf ];
    void* dstBytes = ( char* )dynBuf.mMappedCPUAddr + params.mByteOffset;

    MemCpy( dstBytes, srcBytes, srcByteCount );
  }

  void DX12Backend::SetRenderObjectName( const SetRenderObjectNameParams& params )
  {
    const int i = params.mHandle.GetHandleIndex();
    const HandleType type = params.mHandle.GetHandleType();
    switch( type )
    {
    case HandleType::kDynamicBuffer: mDynBufs[ i ].SetName( params.mName ); break;
    case HandleType::kContext: mContexts[ i ].SetName( params.mName ); break;
    }
  }
#endif

  void DX12Backend::CreateFB( FBHandle h,
                              FrameBufferParams params,
                              Errors& errors )
  {
    const void* nwh = params.mNWH;
    const v2i size = params.mSize;
    const int iHandle = h.GetIndex();

    DX12CommandQueue cmdQ;
    cmdQ.Create( mDevice.GetID3D12Device(), errors );


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
      .mDevice = ( IUnknown* )cmdQ.GetCommandQueue(), // swap chain can force flush the queue
      .mBufferCount = TAC_SWAP_CHAIN_BUF_COUNT,
      .mWidth = size.x,
      .mHeight = size.y,
      .mFmt = fmt,
    };
    TAC_ASSERT( scInfo.mDevice );

    PCom< IDXGISwapChain4 > swapChain = TAC_CALL( DXGICreateSwapChain( scInfo, errors ) );
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    TAC_CALL( swapChain->GetDesc1( &swapChainDesc ) );

    SwapChainRTVs rtvs;
    for( UINT iSwapChainBuf = 0; iSwapChainBuf < TAC_SWAP_CHAIN_BUF_COUNT; iSwapChainBuf++ )
    {
      const int iRTVDescriptor = iHandle * TAC_SWAP_CHAIN_BUF_COUNT;
      const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle
        = mRTVDescriptorHeap.IndexCPUDescriptorHandle( iRTVDescriptor );

      PCom< ID3D12Resource > renderTarget;
      TAC_DX12_CALL( swapChain->GetBuffer(
        iSwapChainBuf,
        renderTarget.iid(),
        renderTarget.ppv() ) );

      ID3D12Device* device = mDevice.GetID3D12Device();
      device->CreateRenderTargetView( ( ID3D12Resource* )renderTarget, nullptr, rtvHandle );

      DX12SetName( renderTarget, "Render Target " + Tac::ToString( iHandle ) );

      rtvs[ iSwapChainBuf ] = DX12Resource
      {
        .mResource = renderTarget,
        .mDesc = renderTarget->GetDesc(),

        // the render target resource is created in a state that is ready to be displayed on screen
        .mState = D3D12_RESOURCE_STATE_PRESENT,
      };
    }

    mFrameBufs[ iHandle ] = DX12FrameBuf
    {
      .mNWH = nwh,
      .mSize = size,
      .mCommandQueue = ( DX12CommandQueue&& )cmdQ,
      .mSwapChain = swapChain,
      .mSwapChainDesc = swapChainDesc,
      .mRTVs = rtvs,
      .mFmt = params.mColorFmt,
    };
  }

  void DX12Backend::ResizeFB( FBHandle h, v2i size )
  {
    DX12FrameBuf& frameBuf = mFrameBufs[ h.GetIndex() ];
    if( frameBuf.mSize == size )
      return;

    OS::OSDebugBreak();

  }

  TexFmt DX12Backend::GetFBFmt( FBHandle h )
  {
    return mFrameBufs[ h.GetIndex() ].mFmt;
  }

  void DX12Backend::DestroyFB( FBHandle h )
  {
    if( h.IsValid() )
    {
      mFrameBufs[ h.GetIndex() ] = {};
    }
  }

  void DX12Backend::CreateDynBuf( DynBufHandle h,
                                  int byteCount,
                                  StackFrame sf,
                                  Errors& errors )
  {
    const D3D12_HEAP_PROPERTIES HeapProps
    {
      .Type = D3D12_HEAP_TYPE_UPLOAD,
      .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
      .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
      .CreationNodeMask = 1,
      .VisibleNodeMask = 1,
    };

    const D3D12_RESOURCE_DESC ResourceDesc
    {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
      .Alignment = 0,
      .Width = ( UINT64 )byteCount,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc = DXGI_SAMPLE_DESC
      {
        .Count = 1,
        .Quality = 0
      },
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    const D3D12_RESOURCE_STATES DefaultUsage{ D3D12_RESOURCE_STATE_GENERIC_READ };

    ID3D12Device* device = mDevice.GetID3D12Device();

    PCom< ID3D12Resource > buffer;
    TAC_DX12_CALL( device->CreateCommittedResource(
      &HeapProps,
      D3D12_HEAP_FLAG_NONE,
      &ResourceDesc,
      DefaultUsage,
      nullptr,
      buffer.iid(),
      buffer.ppv() ) );

    DX12SetName( buffer, sf );

    void* cpuAddr;

    TAC_DX12_CALL( buffer->Map(
      0, // subrsc idx
      nullptr, // nullptr indicates the whole subrsc may be read by cpu
      &cpuAddr ) );

    const int i = h.GetIndex();

    DX12Resource rsc
    {
      .mResource = buffer,
    };

    mDynBufs[ i ] = DX12DynBuf
    {
      .mResource = rsc,
      .mMappedCPUAddr = cpuAddr,
    };
  }

  void DX12Backend::UpdateDynBuf( RenderApi::UpdateDynBufParams params )
  {
    DynBufHandle h = params.mHandle;
    DX12DynBuf& dynBuf = mDynBufs[ h.GetIndex() ];
    char* dstBytes = ( char* )dynBuf.mMappedCPUAddr + params.mDstByteOffset;
    MemCpy( dstBytes, params.mSrcBytes, params.mSrcByteCount );
  }

  void DX12Backend::DestroyDynBuf( DynBufHandle h )
  {
    if( h.IsValid() )
    {
      mDynBufs[ h.GetIndex() ] = {};
    }
  }

  void DX12Backend::CreateShaderProgram( ProgramHandle h,
                                         ShaderProgramParams params,
                                         Errors& errors )
  {

    mShaderProgramMgr.CreateShaderProgram( h, params, errors );

  }

  void DX12Backend::DestroyShaderProgram( ProgramHandle h )
  {
  }

  void DX12Backend::CreateRenderPipeline( PipelineHandle h,
                                          PipelineParams params,
                                          Errors& errors )
  {
    auto ToDxgiFormat = []( TexFmt fmt )
      {
        switch( fmt )
        {
        case Tac::Render::kUnknown: return DXGI_FORMAT_UNKNOWN;
        case Tac::Render::kD24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case Tac::Render::kRGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        default: TAC_ASSERT_INVALID_CASE( fmt ); return DXGI_FORMAT_UNKNOWN;
        }
      };

    ID3D12Device* device = mDevice.GetID3D12Device();

    DX12ShaderProgram* program = mShaderProgramMgr.FindProgram( params.mProgram );

    const D3D12_RASTERIZER_DESC RasterizerState
    {
      .FillMode = D3D12_FILL_MODE_SOLID,
      .CullMode = D3D12_CULL_MODE_BACK,
      .FrontCounterClockwise = true,
      .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
      .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
      .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
      .DepthClipEnable = true,
    };

    const D3D12_BLEND_DESC BlendState
    {
      .RenderTarget =
      {
        D3D12_RENDER_TARGET_BLEND_DESC
        {
          .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
        },
      },
    };

    const DXGI_SAMPLE_DESC SampleDesc{ .Count = 1 };

    OS::OSDebugBreak();
    //device->CreateRootSignature( ... );


    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
    {
      //.pRootSignature = ( ID3D12RootSignature* )m_rootSignature,
      .VS = program->mVSBytecode,
      .PS = program->mPSBytecode,
      .BlendState = BlendState,
      .SampleMask = UINT_MAX,
      .RasterizerState = RasterizerState,
      .DepthStencilState = D3D12_DEPTH_STENCIL_DESC{},
      .InputLayout = D3D12_INPUT_LAYOUT_DESC{},
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .DSVFormat = ToDxgiFormat( params.mDSVDepthFmt ),
      .SampleDesc = SampleDesc,
    };


    const int n  = params.mRTVColorFmts.size(); 
    for( int i = 0; i < n; ++i )
      psoDesc.RTVFormats[i] = ToDxgiFormat( params.mRTVColorFmts[i] );


    PCom< ID3D12PipelineState > pso;
    TAC_CALL( device->CreateGraphicsPipelineState( &psoDesc, pso.iid(), pso.ppv() ) );

    DX12SetName( pso, "Pipeline State" + Tac::ToString( h.GetIndex() ) );

    mPipelines[ h.GetIndex() ] = DX12Pipeline
    {
      .mPSO = pso,
    };
  }

  void DX12Backend::DestroyRenderPipeline( PipelineHandle h )
  {
  }

} // namespace Tac::Render
