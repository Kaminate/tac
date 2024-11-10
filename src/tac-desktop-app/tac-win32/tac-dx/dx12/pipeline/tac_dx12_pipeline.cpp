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
    mRootParameterBinding->mBindlessArray = bindlessArray;
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

  StringView             DX12Pipeline::Variable::GetName() const
  {
    return mRootParameterBinding->mProgramBindDesc.mName;
  }

#if 0
  Span< DX12Descriptor > DX12Pipeline::Variable::GetDescriptors(
    DX12TransitionHelper* transitionHelper ) const
  {
    return mRootParameterBinding->GetDescriptors( transitionHelper );
  }
#endif




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

