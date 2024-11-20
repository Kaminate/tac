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
  // bindless, DX12DescriptorRegion persists between draw calls
  // ( as oppposed to a PipelineDynamicArray, where a DX12DescriptorRegion is allocated per call )
  struct BindlessArray : public IBindlessArray
  {
    BindlessArray( IBindlessArray::Params );
    Binding                Bind( ResourceHandle, Errors& ) override;
    void                   Unbind( Binding ) override;
    void                   Resize( int );
    void                   SetFenceSignal( FenceSignal );
    //Span< DX12Descriptor > GetDescriptors( DX12TransitionHelper* ) const override;
    void                   Commit( CommitParams );

  private:

    void                   CheckType( ResourceHandle );

    Vector< IHandle >      mHandles          {};
    Vector< Binding >      mUnusedBindings   {};
    DX12DescriptorRegion   mDescriptorRegion {};
    FenceSignal            mFenceSignal      {};
    //D3D12ProgramBindDesc mProgramBindDesc  {};
    D3D12ProgramBindType   mProgramBindType  {};
  };

} // namespace Tac::Render

