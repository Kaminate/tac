// The purpose of this file is to keep track of what resources are
// bound to 
//
// 

#pragma once

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h" // DX12DescriptorRegion
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_bindless_array.h"
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_dynamic_array.h"
#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{

  struct RootParameterBinding
  {
    enum class Type
    {
      kUnknown = 0,
      kDynamicArray,
      kBindlessArray,
      kResourceHandle,
    };
    //Span< DX12Descriptor > GetDescriptors( DX12TransitionHelper* ) const;
    void                   SetFence( FenceSignal );

    void                   Commit( CommitParams );
  private:
    void                   CommitAsDescriptorTable( CommitParams );
    void                   CommitAsResource( CommitParams );
  public:


    // Resources can be bound in an array, or as a handle, depending
    // on the resource type
    PipelineDynamicArray  mPipelineDynamicArray {};
    IBindlessArray*       mBindlessArray        {}; // not owned
    ResourceHandle        mResourceHandle       {};

    D3D12ProgramBindDesc  mProgramBindDesc      {};
    UINT                  mRootParameterIndex   {};
    Type                  mType                 {};
  };

  struct PipelineBindCache : public Vector< RootParameterBinding >
  {
    PipelineBindCache() = default;
    PipelineBindCache( const D3D12ProgramBindDescs& );
  };

} // namespace Tac::Render

