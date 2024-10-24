#include "tac_dx12_pipeline.h" // self-inc

#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-dx/dx12/sampler/tac_dx12_sampler_mgr.h"
#include "tac-dx/dx12/buffer/tac_dx12_buffer_mgr.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"

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


  // -----------------------------------------------------------------------------------------------

  ctor                   DX12Pipeline::Variable::Variable( UINT rootParameterIndex,
                                                           D3D12ProgramBindDesc binding )
    : mRootParameterIndex( rootParameterIndex )
    , mBinding( binding )
  {
    //mHandleIndexes.resize( binding.mBindCount, -1 );
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void                   DX12Pipeline::Variable::SetResource( ResourceHandle h )
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void                   DX12Pipeline::Variable::SetResourceAtIndex( int i, ResourceHandle h )
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

  PipelineArray*         DX12Pipeline::Variable::GetPipelineArray()
  {
    TAC_ASSERT_UNIMPLEMENTED;
    return {};
  }

  //void DX12Pipeline::Variable::SetBuffer( BufferHandle h )
  //{
  //  TAC_ASSERT( mBinding.mType. IsBuffer() );
  //  SetElement( h.GetIndex() );
  //}

  //void DX12Pipeline::Variable::SetTexture( TextureHandle h )
  //{
  //  TAC_ASSERT( mBinding.mType.IsTexture() );
  //  SetElement( h.GetIndex() );
  //}

  //void DX12Pipeline::Variable::SetBufferAtIndex( int i, BufferHandle h )
  //{
  //  TAC_ASSERT( mBinding.mType.IsBuffer() );
  //  SetArrayElement( i, h.GetIndex() );
  //}

  //void DX12Pipeline::Variable::SetTextureAtIndex( int i, TextureHandle h )
  //{
  //  TAC_ASSERT( mBinding.mType.IsTexture() );
  //  SetArrayElement( i, h.GetIndex() );
  //}

  //void DX12Pipeline::Variable::SetSamplerAtIndex( int i, SamplerHandle h )
  //{
  //  TAC_ASSERT( mBinding.mType.IsSampler() );
  //  SetArrayElement( i, h.GetIndex() );
  //}

  void                   DX12Pipeline::Variable::SetBindlessArray( IShaderBindlessArray* )
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

  //void DX12Pipeline::Variable::SetSampler( SamplerHandle h )
  //{
  //  TAC_ASSERT( mBinding.mType.IsSampler() );
  //  SetElement( h.GetIndex() );
  //}

  //void DX12Pipeline::Variable::SetElement( int iHandle )
  //{
  //  TAC_ASSERT( mHandleIndexes.size() == 1 );
  //  TAC_ASSERT( mBinding.mBindCount == 1 );
  //  mHandleIndexes[ 0 ] = iHandle;
  //}

  void                   DX12Pipeline::Variable::SetArrayElement( int iArray, IHandle h )
  {
    //TAC_ASSERT_INDEX( iArray, 1000 ); // sanity
    //TAC_ASSERT( mBinding.mBindCount >= 0 ); // sanity

    //// resize unbounded array
    //if( !mBinding.mBindCount && iArray >= mHandleIndexes.size() )
    //  mHandleIndexes.resize( iArray + 1, -1 );

    //mHandleIndexes[ iArray ] = h;

    TAC_ASSERT_UNIMPLEMENTED;
  }

  StringView             DX12Pipeline::Variable::GetName() const
  {
    return mBinding.mName;
  }

  Span< DX12Descriptor > DX12Pipeline::Variable::GetDescriptors(
    DX12TransitionHelper* transitionHelper ) const
  {

    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    DX12SamplerMgr* samplerMgr { &renderer.mSamplerMgr };

    const int n{ mHandleIndexes.size() };
    DX12Descriptor* dst{
      ( DX12Descriptor* )FrameMemoryAllocate( sizeof( DX12Descriptor ) * n ) };

    Span< DX12Descriptor > result( dst, n );

    for( IHandle iHandle : mHandleIndexes )
    {
      DX12Descriptor descriptor{
        GetDescriptor( iHandle, transitionHelper ) };
      TAC_ASSERT( descriptor.IsValid() );
      *dst++ = descriptor;
    }

    return result;
  }

  DX12Descriptor         DX12Pipeline::Variable::GetDescriptor(
    IHandle ih,
    DX12TransitionHelper* transitionHelper ) const
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    DX12SamplerMgr* samplerMgr { &renderer.mSamplerMgr };

    const D3D12ProgramBindDesc binding{ mBinding };
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

  const D3D12ProgramBindDesc& DX12Pipeline::Variable::GetBinding() const
  {
    return mBinding;
  }

  void                   DX12Pipeline::Variable::Commit( CommitParams commitParams ) const
  {
    ID3D12GraphicsCommandList* commandList{ commitParams.mCommandList };
    DX12DescriptorCaches* descriptorCaches{ commitParams.mDescriptorCaches };
    const bool isCompute{ commitParams.mIsCompute };

    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    DX12SamplerMgr* samplerMgr { &renderer.mSamplerMgr };
    ID3D12Device*   device     { renderer.mDevice };

    if( mBinding.BindsAsDescriptorTable() )
    {
      const UINT rootParameterIndex{ mRootParameterIndex };
      const D3D12_DESCRIPTOR_HEAP_TYPE heapType{ GetHeapType( mBinding.mType ) };

      DX12TransitionHelper transitionHelper;
      const Span< DX12Descriptor > cpuDescriptors{ GetDescriptors( &transitionHelper ) };
      transitionHelper.ResourceBarrier( commandList );

      dynmc DX12DescriptorCache& descriptorCache{ ( *descriptorCaches )[ heapType ] };
      const DX12DescriptorRegion* gpuDescriptor{
        descriptorCache.GetGPUDescriptorForCPUDescriptors( cpuDescriptors ) };

      const int nDescriptors{ cpuDescriptors.size() };
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
      }

      const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ gpuDescriptor->GetGPUHandle() };
      if( isCompute )
        commandList->SetComputeRootDescriptorTable( rootParameterIndex, gpuHandle );
      else
        commandList->SetGraphicsRootDescriptorTable( rootParameterIndex, gpuHandle );
    }
    else
    {
      const UINT rootParameterIndex{ mRootParameterIndex };
      TAC_ASSERT( mHandleIndexes.size() == 1 );
      const BufferHandle bufferHandle{ mHandleIndexes[ 0 ] };

      const D3D12ProgramBindType programBindType{mBinding.mType};

      TAC_ASSERT_MSG( !programBindType.IsTexture(),
                      "textures must be bound thorugh descriptor tables" );

      // this includes constant buffers
      TAC_ASSERT( programBindType.IsBuffer() );

      const DX12Buffer* buffer{ bufferMgr->FindBuffer( bufferHandle ) };
      TAC_ASSERT( buffer );
      
      const D3D12_RESOURCE_STATES state{ buffer->mResource.GetState() };

      const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress{ buffer->mGPUVirtualAddr };
      TAC_ASSERT( gpuVirtualAddress );

      using CmdList = ID3D12GraphicsCommandList;
      using CmdListFn = void ( CmdList::* )( UINT, D3D12_GPU_VIRTUAL_ADDRESS );

      CmdListFn cmdListFn{};

      if( programBindType.IsConstantBuffer() )
      {
        TAC_ASSERT( state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
        cmdListFn = isCompute
          ? &CmdList::SetComputeRootConstantBufferView
          : &CmdList::SetGraphicsRootConstantBufferView;
      }
      else if( programBindType.IsSRV() )
      {
        TAC_ASSERT( state & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );

        cmdListFn = isCompute
          ? &CmdList::SetComputeRootShaderResourceView
          : &CmdList::SetGraphicsRootShaderResourceView;
      }
      else if( programBindType.IsUAV() )
      {
        TAC_ASSERT( state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
        cmdListFn = isCompute
          ? &CmdList::SetComputeRootUnorderedAccessView
          : &CmdList::SetGraphicsRootUnorderedAccessView;
      }

      TAC_ASSERT( cmdListFn );
      ( commandList->*cmdListFn )( rootParameterIndex, gpuVirtualAddress );
    }
  }

  // -----------------------------------------------------------------------------------------------

  DX12Pipeline::Variables::Variables( const D3D12ProgramBindDescs& bindings )
  {
    UINT rootParamIndex{};
    for( const D3D12ProgramBindDesc& binding : bindings )
      push_back( DX12Pipeline::Variable( rootParamIndex++, binding ) );
  }

  
  // -----------------------------------------------------------------------------------------------
  // -----------------------------------------------------------------------------------------------

  bool DX12Pipeline:: IsValid() const
  {
    return mPSO.Get() != nullptr;
  }

} // namespace Tac::Render

