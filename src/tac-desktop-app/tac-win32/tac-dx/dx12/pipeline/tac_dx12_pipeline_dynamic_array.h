// The purpose of this file is ...
//
// 

#pragma once

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h" // DX12DescriptorRegion
//#include "tac-dx/dx12/program/tac_dx12_program_bind_type.h"
#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_commit_params.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{

  // A new DX12DescriptorRegion is allocated for each draw call
  struct PipelineDynamicArray // : public IPipelineArray
  {
    Span< DX12Descriptor > GetDescriptors( DX12TransitionHelper* ) const; // override;
    void                   BindAtIndex( ResourceHandle, int );
    //void                   SetFence( FenceSignal );
    void                   Commit( CommitParams );

  // private:
    void                   CheckType( ResourceHandle );
    DX12Descriptor         GetDescriptor( IHandle, DX12TransitionHelper* ) const;

    Vector< IHandle >      mHandleIndexes        {};
    D3D12ProgramBindType   mProgramBindType      {};

    // Temporary per draw call, allocated during Commit(), and freed during SetFence()
    //DX12DescriptorRegion   mDescriptorRegion     {};

    int                    mMaxBoundIndex        { -1 };
  };




} // namespace Tac::Render

