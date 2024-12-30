#include "tac_dx12_pipeline_bind_cache.h" // self-inc

#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac::Render
{

  using CmdList = ID3D12GraphicsCommandList;
  using CmdListFn = void ( CmdList::* )( UINT, D3D12_GPU_VIRTUAL_ADDRESS );

  static CmdListFn GetCmdListFn( D3D12ProgramBindType programBindType,
                                 D3D12_RESOURCE_STATES state,
                                 CommitParams commitParams )
  {
    const bool isCompute{ commitParams.mIsCompute };
    if( programBindType.IsConstantBuffer() )
    {
      TAC_ASSERT( state & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
      return isCompute
        ? &CmdList::SetComputeRootConstantBufferView
        : &CmdList::SetGraphicsRootConstantBufferView;
    }

    if( programBindType.IsSRV() )
    {
      TAC_ASSERT( state & D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE );
      return isCompute
        ? &CmdList::SetComputeRootShaderResourceView
        : &CmdList::SetGraphicsRootShaderResourceView;
    }

    if( programBindType.IsUAV() )
    {
      TAC_ASSERT( state & D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
      return isCompute
        ? &CmdList::SetComputeRootUnorderedAccessView
        : &CmdList::SetGraphicsRootUnorderedAccessView;
    }

    TAC_ASSERT_INVALID_CODE_PATH;
    return nullptr;
  }

  // -----------------------------------------------------------------------------------------------

  void           RootParameterBinding::SetFence( FenceSignal fenceSignal )
  {
    // | commented out because now its part of dx12descriptorcache
    // |
    // v
    //if( mType == Type::kDynamicArray )
    //{
    //  mPipelineDynamicArray.SetFence( fenceSignal );
    //}
    if( mType == Type::kBindlessArray )
    {
      // todo: cleanup
      BindlessArray* bindlessArray{ ( BindlessArray* )mBindlessArray };
      bindlessArray->SetFenceSignal( fenceSignal );
      return;
    }

    {
      // ???
    }
  }

  void           RootParameterBinding::CommitAsDescriptorTable( CommitParams commitParams)
  {
    switch( mType )
    {
    case Type::kDynamicArray:
      mPipelineDynamicArray.Commit( commitParams );
      break;
    case Type::kBindlessArray :
      // gross gross gross gross gross gross gross gross 
      ( ( BindlessArray* )mBindlessArray )->Commit( commitParams );
      break;
    default:
      TAC_ASSERT_INVALID_CASE( mType );
      break;
    }
  }

  void           RootParameterBinding::CommitAsResource( CommitParams commitParams)
  {
    const D3D12ProgramBindType programBindType{ mProgramBindDesc.mType };
    TAC_ASSERT_MSG( !programBindType.IsTexture(),
                    "Textures must be bound through descriptor tables" );
    TAC_ASSERT( programBindType.IsBuffer() ); // this includes constant buffers
    const bool isCompute{ commitParams.mIsCompute };
    const BufferHandle bufferHandle{ mResourceHandle };
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12BufferMgr* bufferMgr{ &renderer.mBufMgr };
    const DX12Buffer* buffer{ bufferMgr->FindBuffer( bufferHandle ) };
    TAC_ASSERT( buffer );
    const D3D12_RESOURCE_STATES state{ buffer->mResource.GetState() };
    const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress{ buffer->mGPUVirtualAddr };
    TAC_ASSERT( gpuVirtualAddress );
    const CmdListFn cmdListFn{ GetCmdListFn( programBindType, state, commitParams ) };
    TAC_ASSERT( cmdListFn );
    ID3D12GraphicsCommandList* commandList{ commitParams.mCommandList };
    ( commandList->*cmdListFn )( mRootParameterIndex, gpuVirtualAddress );
  }

  void           RootParameterBinding::Commit( CommitParams commitParams )
  {
    if( mProgramBindDesc.BindsAsDescriptorTable() )
      CommitAsDescriptorTable( commitParams );
    else
      CommitAsResource( commitParams );
  }

  // -----------------------------------------------------------------------------------------------

  PipelineBindCache::PipelineBindCache( const D3D12ProgramBindDescs& descs )
  {
    const int n{ descs.size() };

    for( int i{}; i < n; ++i )
    {
      const D3D12ProgramBindDesc& desc{ descs[ i ] };

      //PipelineDynamicArray pipelineDynamicArray;
      //if( desc.BindsAsDescriptorTable() && desc.mBindCount > 0 )
      //{
      //}

      RootParameterBinding rootParameterBinding;
      rootParameterBinding.mProgramBindDesc = desc;
      rootParameterBinding.mRootParameterIndex = i;
      //rootParameterBinding.mPipelineDynamicArray = ( PipelineDynamicArray&& )pipelineDynamicArray;
      rootParameterBinding.mPipelineDynamicArray.mProgramBindType = desc.mType;
      if( desc.mBindCount > 0 )
        rootParameterBinding.mPipelineDynamicArray.mHandleIndexes.resize( desc.mBindCount );
      rootParameterBinding.mType = desc.BindsAsDescriptorTable()
        ? RootParameterBinding::Type::kDynamicArray
        : RootParameterBinding::Type::kResourceHandle;

      //if( desc.mBindCount

      push_back( ( RootParameterBinding&& )rootParameterBinding );
    }
  }

} // namespace Tac::Render

