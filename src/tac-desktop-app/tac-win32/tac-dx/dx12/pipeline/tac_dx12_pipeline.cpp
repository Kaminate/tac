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
    //: mRootParameterIndex( rootParameterIndex )
    //, mBinding( binding )
  {
    //mHandleIndexes.resize( binding.mBindCount, -1 );
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void                   DX12Pipeline::Variable::SetResource( ResourceHandle h ) 
  {
    mRootParameterBinding->mType = RootParameterBinding::Type::kResourceHandle;
    mRootParameterBinding->mResourceHandle = h;
  }

  void                   DX12Pipeline::Variable::SetResourceAtIndex( int i, ResourceHandle h )
  {
    mRootParameterBinding->mType = RootParameterBinding::Type::kDynamicArray; // ?!
    mRootParameterBinding->mPipelineDynamicArray;
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void                   DX12Pipeline::Variable::SetBindlessArray(
    IShaderBindlessArray* bindlessArray )
  {
    mRootParameterBinding->mPipelineArray = bindlessArray;
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
    return mRootParameterBinding->mProgramBindDesc.mName;
  }

  Span< DX12Descriptor > DX12Pipeline::Variable::GetDescriptors(
    DX12TransitionHelper* transitionHelper ) const
  {
    return mRootParameterBinding->GetDescriptors( transitionHelper );
  }


  void                   DX12Pipeline::Variable::Commit( CommitParams commitParams ) const
  {
    TAC_ASSERT(mRootParameterBinding->mType == RootParameterBinding::Type::kDynamicArray );

    const D3D12ProgramBindDesc& programBindDesc{ mRootParameterBinding->mProgramBindDesc };
    ID3D12GraphicsCommandList* commandList{ commitParams.mCommandList };
    DX12DescriptorCaches* descriptorCaches{ commitParams.mDescriptorCaches };
    const bool isCompute{ commitParams.mIsCompute };
    const UINT rootParameterIndex{ mRootParameterBinding->mRootParameterIndex };
    const D3D12ProgramBindType programBindType{ programBindDesc.mType };

    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12TextureMgr* textureMgr { &renderer.mTexMgr };
    DX12BufferMgr*  bufferMgr  { &renderer.mBufMgr };
    DX12SamplerMgr* samplerMgr { &renderer.mSamplerMgr };
    ID3D12Device*   device     { renderer.mDevice };

    if( programBindDesc.BindsAsDescriptorTable() )
    {
      const D3D12_DESCRIPTOR_HEAP_TYPE heapType{ GetHeapType( programBindType ) };

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
      TAC_ASSERT( mHandleIndexes.size() == 1 );
      const BufferHandle bufferHandle{ mHandleIndexes[ 0 ] };


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

