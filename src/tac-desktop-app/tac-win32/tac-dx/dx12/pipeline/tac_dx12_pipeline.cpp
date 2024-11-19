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



  // -----------------------------------------------------------------------------------------------

  //ctor                   DX12Pipeline::Variable::Variable( UINT rootParameterIndex,
  //                                                         D3D12ProgramBindDesc binding )
    //: mRootParameterIndex( rootParameterIndex )
    //, mBinding( binding )
  //{
  //  //mHandleIndexes.resize( binding.mBindCount, -1 );
  //  TAC_ASSERT_UNIMPLEMENTED;
  //}

  void                   DX12Pipeline::Variable::SetResource( ResourceHandle h ) 
  {
    RootParameterBinding* rootParameterBinding{ GetRootParameterBinding() };
    TAC_ASSERT( rootParameterBinding->mType == RootParameterBinding::Type::kResourceHandle );
    rootParameterBinding->mResourceHandle = h;
  }

  void                   DX12Pipeline::Variable::SetResourceAtIndex( ResourceHandle h, int i )
  {
    RootParameterBinding* rootParameterBinding{ GetRootParameterBinding() };
    TAC_ASSERT( rootParameterBinding->mType == RootParameterBinding::Type::kDynamicArray );
    rootParameterBinding->mPipelineDynamicArray.BindAtIndex( h, i );
  }

  void                   DX12Pipeline::Variable::SetBindlessArray(
    IBindlessArray* bindlessArray )
  {
    RootParameterBinding* rootParameterBinding{ GetRootParameterBinding() };
    rootParameterBinding->mBindlessArray = bindlessArray;
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

  //void                   DX12Pipeline::Variable::SetArrayElement( int iArray, IHandle h )
  //{
    //TAC_ASSERT_INDEX( iArray, 1000 ); // sanity
    //TAC_ASSERT( mBinding.mBindCount >= 0 ); // sanity

    //// resize unbounded array
    //if( !mBinding.mBindCount && iArray >= mHandleIndexes.size() )
    //  mHandleIndexes.resize( iArray + 1, -1 );

    //mHandleIndexes[ iArray ] = h;

  //  TAC_ASSERT_UNIMPLEMENTED;
  //}

  RootParameterBinding*  DX12Pipeline::Variable::GetRootParameterBinding() const
  {
    DX12Renderer&    renderer   { DX12Renderer::sRenderer };
    DX12PipelineMgr& pipelineMgr{ renderer.mPipelineMgr };
    DX12Pipeline*    pipeline   { pipelineMgr.FindPipeline( mPipelineHandle ) };
    TAC_ASSERT( pipeline );
    return &pipeline->mPipelineBindCache[ mRootParameterIndex ];
  }

  StringView             DX12Pipeline::Variable::GetName() const
  {
    RootParameterBinding* rootParameterBinding{ GetRootParameterBinding() };
    return rootParameterBinding->mProgramBindDesc.mName;
  }

#if 0
  Span< DX12Descriptor > DX12Pipeline::Variable::GetDescriptors(
    DX12TransitionHelper* transitionHelper ) const
  {
    return mRootParameterBinding->GetDescriptors( transitionHelper );
  }
#endif




  // -----------------------------------------------------------------------------------------------

  DX12Pipeline::Variables::Variables( PipelineHandle h, int n)
  {
    for( int i{}; i < n; ++i )
    {
      DX12Pipeline::Variable variable;
      variable.mPipelineHandle = h;
      variable.mRootParameterIndex = i;
      push_back( variable );
    }
  }

#if 0
  DX12Pipeline::Variables::Variables( const D3D12ProgramBindDescs& bindings )
  {
    UINT rootParamIndex{};
    for( const D3D12ProgramBindDesc& binding : bindings )
      push_back( DX12Pipeline::Variable( rootParamIndex++, binding ) );
  }
#else
  //DX12Pipeline::Variables::Variables(const PipelineBindCache& cache )
  //{
  //  const int n {cache.size()};
  //  reserve(n);
  //  for( int i{}; i < n; ++i )
  //  {
  //    RootParameterBinding* binding{ &cache[ i ] };
  //    DX12Pipeline::Variable var;
  //    var.mRootParameterBinding = 
  //    push_back( DX12Pipeline::Variable{ .mRootParameterBinding{  } } );
  //  }

  //  UINT rootParamIndex{};
  //  for( const D3D12ProgramBindDesc& binding : bindings )
  //    push_back( DX12Pipeline::Variable( rootParamIndex++, binding ) );
  //}
#endif

  
  // -----------------------------------------------------------------------------------------------
  // -----------------------------------------------------------------------------------------------

  bool DX12Pipeline:: IsValid() const
  {
    return mPSO.Get() != nullptr;
  }

} // namespace Tac::Render

