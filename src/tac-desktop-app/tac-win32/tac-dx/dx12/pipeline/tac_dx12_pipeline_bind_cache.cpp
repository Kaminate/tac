#include "tac_dx12_pipeline_bind_cache.h" // self-inc

#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------
  //                     PipelineBindlessArray
  // -----------------------------------------------------------------------------------------------

  void                    PipelineBindlessArray::CheckType( ResourceHandle h )
  {
    if constexpr ( !kIsDebugMode )
      return;

    const D3D12ProgramBindType type{ mProgramBindType };
    TAC_ASSERT( type.IsValid() );
    TAC_ASSERT( !type.IsBuffer() || h.IsBuffer() );
    TAC_ASSERT( !type.IsTexture() || h.IsTexture() );
    TAC_ASSERT( !type.IsSampler() || h.IsSampler() );
  }

  PipelineBindlessArray::HeapType PipelineBindlessArray::GetHeapType() const
  {
    const D3D12ProgramBindType type{ mProgramBindType };

    if( type.IsBuffer() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    if( type.IsTexture() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    if( type.IsSampler() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    TAC_ASSERT_INVALID_CODE_PATH;
    return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
  }

  void                    PipelineBindlessArray::SetFenceSignal( FenceSignal fenceSignal )
  {
    mFenceSignal = fenceSignal;
  }

  void                    PipelineBindlessArray::Resize( const int newSize )
  {
    const int oldSize{ mHandles.size() };

    TAC_ASSERT( mUnusedBindings.empty() );
    TAC_ASSERT( newSize > oldSize );

    mHandles.resize( newSize );
    for( int i{ oldSize }; i < newSize; ++i )
      mUnusedBindings.push_back( Binding{ i } );

    const HeapType heapType{ GetHeapType() };
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12DescriptorHeap& heap{ renderer.mGpuDescriptorHeaps[ heapType ] };
    DX12DescriptorAllocator* regionMgr{ heap.GetRegionMgr() };


    DX12DescriptorRegion newRegion{ regionMgr->Alloc( newSize ) };

    ID3D12Device* device{ renderer.mDevice };
    device->CopyDescriptorsSimple( oldSize,
                                   newRegion.GetCPUHandle(),
                                   mDescriptorRegion.GetCPUHandle(),
                                   heapType );

    mDescriptorRegion.Free( mFenceSignal );

    mDescriptorRegion = ( DX12DescriptorRegion&& )newRegion;
  }

  PipelineBindlessArray::Binding  PipelineBindlessArray::Bind( ResourceHandle h )
  {
    CheckType( h );

    if( mUnusedBindings.empty() )
    {
      const int n{ mHandles.size() };
      const Binding binding{ n };
      const int newSize{ ( int )( ( n + 2 ) * 1.5 ) };
      mHandles.resize( newSize );
    }

    const Binding binding{ mUnusedBindings.back() };
    mUnusedBindings.pop_back();
    mHandles[ binding.mIndex ] = h;

    {
    }

    return binding;
  }

  PipelineBindlessArray::Binding  PipelineBindlessArray::BindAtIndex( ResourceHandle h, int i )
  {
    CheckType( h );

    bool bindReady{};
    for( Binding& binding : mUnusedBindings )
    {
      if( binding.mIndex == i )
      {
        Swap( binding, mUnusedBindings.back() );
        mUnusedBindings.pop_back();
        bindReady = true;
      }
    }

    TAC_ASSERT( bindReady );
    mHandles[ i ] = h;

    return Binding{ i };
  }

  void                    PipelineBindlessArray::Unbind( Binding binding )
  {
    mUnusedBindings.push_back( binding );
    mHandles[ binding.mIndex ] = IHandle::kInvalidIndex;
  }


  // ---------------
  Span<DX12Descriptor> RootParameterBinding::GetDescriptors( DX12TransitionHelper* transitionHelper )
  {
    if( mResourceHandle.IsValid() )
    {
      TAC_ASSERT_UNIMPLEMENTED;
      return {};
    }
    else
      return mPipelineArray->GetDescriptors( transitionHelper )

  }

  Span<DX12Descriptor> PipelineDynamicArray::GetDescriptors( DX12TransitionHelper* transitionHelper )
  {
    const int n{ mHandleIndexes.size() };
    DX12Descriptor* dst{
      ( DX12Descriptor* )FrameMemoryAllocate( sizeof( DX12Descriptor ) * n ) };

    Span< DX12Descriptor > result( dst, n );

    for( IHandle iHandle : mHandleIndexes )
    {
      DX12Descriptor descriptor{ GetDescriptor( iHandle, transitionHelper ) };
      TAC_ASSERT( descriptor.IsValid() );
      *dst++ = descriptor;
    }

    return result;
      TAC_ASSERT_UNIMPLEMENTED;
      return{};
  }

  DX12Descriptor PipelineDynamicArray::GetDescriptor( IHandle ih, DX12TransitionHelper* transitionHelper )
  {

    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    DX12SamplerMgr* samplerMgr { &renderer.mSamplerMgr };

    const D3D12ProgramBindDesc& binding{ mProgramBindDesc };

    const D3D12ProgramBindType::Classification classification{ binding.mType.GetClassification() };
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

  Span<DX12Descriptor> PipelineBindlessArray::GetDescriptors( DX12TransitionHelper* transitionHelper)
  {
    TAC_ASSERT_UNIMPLEMENTED;
    return{};

  }


} // namespace Tac::Render

