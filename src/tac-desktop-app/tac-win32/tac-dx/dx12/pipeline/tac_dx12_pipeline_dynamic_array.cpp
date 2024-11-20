#include "tac_dx12_pipeline_dynamic_array.h" // self-inc

#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  static D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType( D3D12ProgramBindType type )
  {
    if( type.IsBuffer() || type.IsTexture() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    if( type.IsSampler() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

    TAC_ASSERT_INVALID_CODE_PATH;
    return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
  }

  static D3D12ProgramBindType::Classification GetClassification( IBindlessArray::Params params )
  {
    const bool isShaderResource{ ( params.mBinding & Binding::ShaderResource ) != Binding::None };
    const bool isBuffer{ params.mHandleType == HandleType::kBuffer };
    const bool isTexture{ params.mHandleType == HandleType::kTexture };

    if( isBuffer && isShaderResource )
      return D3D12ProgramBindType::Classification::kBufferSRV;

    if( isTexture && isShaderResource )
      return D3D12ProgramBindType::Classification::kTextureSRV;

    return D3D12ProgramBindType::Classification::kUnknown;
  }


  // -----------------------------------------------------------------------------------------------

  DX12Descriptor         PipelineDynamicArray::GetDescriptor(
    IHandle ih,
    DX12TransitionHelper* transitionHelper ) const
  {
    DX12Renderer&   renderer   { DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    DX12SamplerMgr* samplerMgr { &renderer.mSamplerMgr };

    const D3D12ProgramBindType::Classification classification{
      mProgramBindType.GetClassification() };
    const int iHandle{ ih.GetIndex() };

    switch( classification )
    {
    case D3D12ProgramBindType::kTextureSRV:
    {
      const TextureHandle textureHandle{ iHandle };
      DX12Texture* texture{ textureMgr->FindTexture( textureHandle ) };
      TAC_ASSERT( texture );
      textureMgr->TransitionResource( &texture->mResource,
                                      Binding::ShaderResource,
                                      transitionHelper );
      return texture->mSRV.GetValue();
    }

    case D3D12ProgramBindType::kTextureUAV:
    {
      const TextureHandle textureHandle{ iHandle };
      DX12Texture* texture{ textureMgr->FindTexture( textureHandle ) };
      TAC_ASSERT( texture );
      textureMgr->TransitionResource( &texture->mResource,
                                      Binding::UnorderedAccess,
                                      transitionHelper );
      TAC_ASSERT( texture->mUAV.HasValue() );
      TAC_ASSERT( texture->mResource.GetState() & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
      return texture->mUAV.GetValue();
    }

    case D3D12ProgramBindType::kBufferSRV:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );
      TAC_ASSERT( buffer->mResource.GetState() & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      // [ ] Q: why the fuck is this calling a textureMgr function?
      //        arent we in the buffermgr?
      textureMgr->TransitionResource( &buffer->mResource,
                                      Binding::ShaderResource,
                                      transitionHelper );
      return buffer->mSRV.GetValue();
    }

    case D3D12ProgramBindType::kBufferUAV:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      const D3D12_RESOURCE_STATES stateBefore{ buffer->mResource.GetState() };
      TAC_ASSERT( buffer );
      TAC_ASSERT( stateBefore & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      // [ ] Q: why the fuck is this calling a textureMgr function?
      //        arent we in the buffermgr?
      textureMgr->TransitionResource( &buffer->mResource,
                                      Binding::UnorderedAccess,
                                      transitionHelper );
      const D3D12_RESOURCE_STATES stateAfter{ buffer->mResource.GetState() };
      TAC_ASSERT( stateAfter & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
      return buffer->mUAV.GetValue();
    }

    case D3D12ProgramBindType::kConstantBuffer:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );
      const D3D12_RESOURCE_STATES stateBefore{ buffer->mResource.GetState() };
      TAC_ASSERT( stateBefore & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      // [ ] Q: why the fuck is this calling a textureMgr function?
      //        arent we in the buffermgr?
      textureMgr->TransitionResource( &buffer->mResource,
                                      Binding::ConstantBuffer,
                                      transitionHelper );
      const D3D12_RESOURCE_STATES stateAfter{ buffer->mResource.GetState() };
      TAC_ASSERT( stateAfter & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
      TAC_ASSERT_UNIMPLEMENTED; // ???
      return {};
    }

    case D3D12ProgramBindType::kSampler:
    {
      DX12Sampler* sampler{ samplerMgr->FindSampler( SamplerHandle{ iHandle } ) };
      TAC_ASSERT( sampler );
      return sampler->mDescriptor;
    }

    default: TAC_ASSERT_INVALID_CASE( classification ); return {};
    }

  }

  Span< DX12Descriptor > PipelineDynamicArray::GetDescriptors(
    DX12TransitionHelper* transitionHelper ) const
  {
    if( mMaxBoundIndex == -1 )
      return {};

    const int nBound{ mMaxBoundIndex + 1 };
    TAC_ASSERT( nBound <= mHandleIndexes.size() );

    DX12Descriptor* dstBegin{
      ( DX12Descriptor* )FrameMemoryAllocate( sizeof( DX12Descriptor ) * nBound ) };
    DX12Descriptor* dstCur{ dstBegin };


    //const IHandle* hBegin{ mHandleIndexes.begin() };

    for( const IHandle& iHandle : Span< const IHandle >( mHandleIndexes.begin(), nBound ) )
    //for( const IHandle* hCur{ hBegin }; hCur < hBegin + nBound; hCur++ ) 
    //for( IHandle iHandle : mHandleIndexes )
    {
      //IHandle iHandle { *hCur };
      TAC_ASSERT( iHandle.IsValid() );

      const DX12Descriptor descriptor{ GetDescriptor( iHandle, transitionHelper ) };
      TAC_ASSERT( descriptor.IsValid() );
      *dstCur++ = descriptor;
    }

    return Span< DX12Descriptor >( dstBegin, nBound );
  }

  void                   PipelineDynamicArray::BindAtIndex( ResourceHandle h, int i )
  {
    CheckType( h );

    if( i >= mHandleIndexes.size() )
    {
      mHandleIndexes.resize( i + 5 ); // magic grow number
    }

    mHandleIndexes[ i ] = h;

    mMaxBoundIndex = Max( mMaxBoundIndex, i );
  }

  void                   PipelineDynamicArray::SetFence( FenceSignal fenceSignal )
  {
    mDescriptorRegion.Free( fenceSignal );
  }

  void                   PipelineDynamicArray::CheckType( ResourceHandle h )
  {
    if constexpr ( !kIsDebugMode )
      return;

    const D3D12ProgramBindType type{ mProgramBindType };
    TAC_ASSERT( type.IsValid() );
    TAC_ASSERT( !type.IsBuffer() || h.IsBuffer() );
    TAC_ASSERT( !type.IsTexture() || h.IsTexture() );
    TAC_ASSERT( !type.IsSampler() || h.IsSampler() );
  }

  void                   PipelineDynamicArray::Commit( CommitParams commitParams )
  {
    ID3D12GraphicsCommandList* commandList{ commitParams.mCommandList };
#if 0
    DX12DescriptorCaches* descriptorCaches{ commitParams.mDescriptorCaches };
#endif
    const bool isCompute{ commitParams.mIsCompute };
    const UINT rootParameterIndex{ commitParams.mRootParameterIndex };

    DX12Renderer&   renderer   { DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    DX12SamplerMgr* samplerMgr { &renderer.mSamplerMgr };
    ID3D12Device*   device     { renderer.mDevice };

    const D3D12_DESCRIPTOR_HEAP_TYPE heapType{ GetHeapType( mProgramBindType ) };

    DX12TransitionHelper transitionHelper;
    const Span< DX12Descriptor > cpuDescriptors{ GetDescriptors( &transitionHelper ) };
    transitionHelper.ResourceBarrier( commandList );

#if 0
    dynmc DX12DescriptorCache& descriptorCache{ ( *descriptorCaches )[ heapType ] };
    const DX12DescriptorRegion* gpuDescriptor{
      descriptorCache.GetGPUDescriptorForCPUDescriptors( cpuDescriptors ) };
#else
    DX12DescriptorHeap& heap{ renderer.mDescriptorHeapMgr.mGPUHeaps[ heapType ] };
    DX12DescriptorAllocator* descriptorAllocator{ heap.GetRegionMgr() };
    const int nDescriptors{ cpuDescriptors.size() };
    mDescriptorRegion = ( DX12DescriptorRegion&& )descriptorAllocator->Alloc( nDescriptors );
    DX12DescriptorRegion* gpuDescriptor{ &mDescriptorRegion };
#endif

    for( int iDescriptor{}; iDescriptor < nDescriptors; ++iDescriptor )
    {
      DX12Descriptor cpuDescriptor { cpuDescriptors[ iDescriptor ] };
      DX12DescriptorHeap* srcHeap{ cpuDescriptor.mOwner };
      TAC_ASSERT( srcHeap );
      TAC_ASSERT( srcHeap->GetType() == heapType );
      const D3D12_CPU_DESCRIPTOR_HANDLE src{ cpuDescriptor.GetCPUHandle() };
      const D3D12_CPU_DESCRIPTOR_HANDLE dst{ gpuDescriptor->GetCPUHandle( iDescriptor ) };
      TAC_ASSERT( src.ptr );
      TAC_ASSERT( dst.ptr );
      device->CopyDescriptorsSimple( 1, dst, src, heapType );

      const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ gpuDescriptor->GetGPUHandle() };
      if( isCompute )
        commandList->SetComputeRootDescriptorTable( rootParameterIndex, gpuHandle );
      else
        commandList->SetGraphicsRootDescriptorTable( rootParameterIndex, gpuHandle );
    }
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac::Render

