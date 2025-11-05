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
  void DX12Pipeline::Variable::SetResource( ResourceHandle h ) 
  {
    RootParameterBinding* rootParameterBinding{ GetRootParameterBinding() };

    if( rootParameterBinding->mProgramBindDesc.BindsAsDescriptorTable() )
    {
      SetResourceAtIndex( h, 0 );
    }
    else
    {
      TAC_ASSERT( rootParameterBinding->mType == RootParameterBinding::Type::kResourceHandle );
      TAC_ASSERT( !rootParameterBinding->mProgramBindDesc.BindsAsDescriptorTable() );
      TAC_ASSERT( !rootParameterBinding->mBindlessArray );
      rootParameterBinding->mResourceHandle = h;
    }
  }

  void DX12Pipeline::Variable::SetResourceAtIndex( ResourceHandle h, int i )
  {
    RootParameterBinding* rootParameterBinding{ GetRootParameterBinding() };
    TAC_ASSERT( rootParameterBinding->mType == RootParameterBinding::Type::kDynamicArray );
    TAC_ASSERT( !rootParameterBinding->mBindlessArray );
    rootParameterBinding->mPipelineDynamicArray.BindAtIndex( h, i );
  }

  void DX12Pipeline::Variable::SetBindlessArray( IBindlessArray* bindlessArray )
  {
    RootParameterBinding* rootParameterBinding{ GetRootParameterBinding() };
    TAC_ASSERT( rootParameterBinding->mProgramBindDesc.BindsAsDescriptorTable() );
    TAC_ASSERT( bindlessArray );
    rootParameterBinding->mBindlessArray = bindlessArray;
    rootParameterBinding->mType = RootParameterBinding::Type::kBindlessArray;
  }

  auto DX12Pipeline::Variable::GetRootParameterBinding() const -> RootParameterBinding*
  {
    DX12Renderer&    renderer   { DX12Renderer::sRenderer };
    DX12PipelineMgr& pipelineMgr{ renderer.mPipelineMgr };
    DX12Pipeline*    pipeline   { pipelineMgr.FindPipeline( mPipelineHandle ) };
    TAC_ASSERT( pipeline );
    return &pipeline->mPipelineBindCache[ mRootParameterIndex ];
  }

  auto DX12Pipeline::Variable::GetName() const -> StringView
  {
    RootParameterBinding* rootParameterBinding{ GetRootParameterBinding() };
    return rootParameterBinding->mProgramBindDesc.mName;
  }

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

  // -----------------------------------------------------------------------------------------------

  bool DX12Pipeline::IsValid() const
  {
    return mPSO.Get() != nullptr;
  }

} // namespace Tac::Render

