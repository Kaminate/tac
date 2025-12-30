#include "tac_dx12_pipeline_dynamic_array.h" // self-inc

#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  static auto GetHeapType( D3D12ProgramBindType type ) -> D3D12_DESCRIPTOR_HEAP_TYPE
  {
    if( type.IsBuffer() || type.IsTexture() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    if( type.IsSampler() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

    TAC_ASSERT_INVALID_CODE_PATH;
    return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
  }

  // -----------------------------------------------------------------------------------------------

  auto PipelineDynamicArray::GetDescriptor(
    IHandle ih,
    DX12TransitionHelper* transitionHelper ) const -> DX12Descriptor
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
      textureMgr->TransitionResource( &texture->mResource, Binding::ShaderResource, transitionHelper );
      return texture->mSRV.GetValue();
    }

    case D3D12ProgramBindType::kTextureUAV:
    {
      const TextureHandle textureHandle{ iHandle };
      DX12Texture* texture{ textureMgr->FindTexture( textureHandle ) };
      TAC_ASSERT( texture );
      textureMgr->TransitionResource( &texture->mResource, Binding::UnorderedAccess, transitionHelper );
      TAC_ASSERT( texture->mUAV.HasValue() );
      TAC_ASSERT( texture->mResource.GetState() & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
      return texture->mUAV.GetValue();
    }

    case D3D12ProgramBindType::kBufferSRV:
    {
      DX12Buffer* buffer{ bufferMgr->FindBuffer( BufferHandle{ iHandle } ) };
      TAC_ASSERT( buffer );
      TAC_ASSERT( buffer->mResource );
      TAC_ASSERT( buffer->mResource.GetState() & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      // [ ] Q: why the fuck is this calling a textureMgr function?
      //        arent we in the buffermgr?
      textureMgr->TransitionResource( &buffer->mResource, Binding::ShaderResource, transitionHelper );
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
      textureMgr->TransitionResource( &buffer->mResource, Binding::UnorderedAccess, transitionHelper );
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
      textureMgr->TransitionResource( &buffer->mResource, Binding::ConstantBuffer, transitionHelper );
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

  auto PipelineDynamicArray::GetDescriptors(
    DX12TransitionHelper* transitionHelper ) const -> Span< DX12Descriptor >
  {
    if( mMaxBoundIndex == -1 )
      return {};

    const int nBound{ mMaxBoundIndex + 1 };
    TAC_ASSERT( nBound <= mHandleIndexes.size() );
    DX12Descriptor* dstBegin{ ( DX12Descriptor* )FrameMemoryAllocate( sizeof( DX12Descriptor ) * nBound ) };
    DX12Descriptor* dstCur{ dstBegin };

    for( const IHandle& iHandle : Span< const IHandle >( mHandleIndexes.begin(), nBound ) )
    {
      TAC_ASSERT( iHandle.IsValid() );
      const DX12Descriptor descriptor{ GetDescriptor( iHandle, transitionHelper ) };
      TAC_ASSERT( descriptor.IsValid() );
      *dstCur++ = descriptor;
    }

    return Span< DX12Descriptor >( dstBegin, nBound );
  }

  void PipelineDynamicArray::BindAtIndex( ResourceHandle h, int i )
  {
    CheckType( h );

    if( i >= mHandleIndexes.size() )
      mHandleIndexes.resize( i + 5 ); // magic grow number

    mHandleIndexes[ i ] = h.GetHandle();
    mMaxBoundIndex = Max( mMaxBoundIndex, i );
  }

  void PipelineDynamicArray::CheckType( ResourceHandle h )
  {
    if constexpr( kIsDebugMode )
    {
      const D3D12ProgramBindType bindType{ mProgramBindType };
      const HandleType handleType{ h.GetHandleType() };
      TAC_ASSERT( bindType.IsValid() );
      TAC_ASSERT( !bindType.IsBuffer() || ( handleType == HandleType::kBuffer ) );
      TAC_ASSERT( !bindType.IsTexture() || ( handleType == HandleType::kTexture ) );
      TAC_ASSERT( !bindType.IsSampler() || ( handleType == HandleType::kSampler ) );
    }
  }

  void PipelineDynamicArray::Commit( CommitParams commitParams )
  {
    ID3D12GraphicsCommandList* commandList{ commitParams.mCommandList };
    const bool isCompute{ commitParams.mIsCompute };
    const UINT rootParameterIndex{ commitParams.mRootParameterIndex };
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    ID3D12Device* device{ renderer.mDevice };

    const D3D12_DESCRIPTOR_HEAP_TYPE heapType{ GetHeapType( mProgramBindType ) };
    dynmc DX12TransitionHelper transitionHelper;
    const Span< DX12Descriptor > srcDescriptors{ GetDescriptors( &transitionHelper ) };
    transitionHelper.ResourceBarrier( commandList );
    dynmc DX12DescriptorHeap& heap{ renderer.mDescriptorHeapMgr.mGPUHeaps[ heapType ] };
    dynmc DX12DescriptorAllocator* descriptorAllocator{ heap.GetGPURegionMgr() };
    const int nDescriptors{ srcDescriptors.size() };
    DX12DescriptorRegion dstDescriptors { descriptorAllocator->Alloc( nDescriptors ) };
    for( int iDescriptor{}; iDescriptor < nDescriptors; ++iDescriptor )
    {
      const DX12Descriptor cpuDescriptor { srcDescriptors[ iDescriptor ] };
      const DX12DescriptorHeap* srcHeap{ cpuDescriptor.mOwner };
      const D3D12_CPU_DESCRIPTOR_HANDLE src{ cpuDescriptor.GetCPUHandle() };
      const D3D12_CPU_DESCRIPTOR_HANDLE dst{ dstDescriptors.GetCPUHandle( iDescriptor ) };
      TAC_ASSERT( srcHeap );
      TAC_ASSERT( srcHeap->GetType() == heapType );
      TAC_ASSERT( src.ptr );
      TAC_ASSERT( dst.ptr );
      device->CopyDescriptorsSimple( 1, dst, src, heapType );
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ dstDescriptors.GetGPUHandle() };
    if( isCompute )
      commandList->SetComputeRootDescriptorTable( rootParameterIndex, gpuHandle );
    else
      commandList->SetGraphicsRootDescriptorTable( rootParameterIndex, gpuHandle );

    commitParams.mDescriptorCache->AddDescriptorRegion( move( dstDescriptors ) );
  }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac::Render

