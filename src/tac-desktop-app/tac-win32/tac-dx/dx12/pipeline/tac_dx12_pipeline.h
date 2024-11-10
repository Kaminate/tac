#pragma once

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_span.h"
#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_bind_cache.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_cache.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h" // DX12Descriptor

#include <d3d12.h> // ID3D12PipelineState

namespace Tac::Render
{
  struct DX12Pipeline
  {
    // The variable is basically an interface into the PipelineBindCache,
    // where each variable (if its a descriptor table) knows its descriptor table index
    // [ ] Q: when is a variable not a descriptor table?
    //   bool D3D12ProgramBindDesc::BindsAsDescriptorTable() const
    struct Variable : public IShaderVar
    {
      //ctor                   Variable() = default;
      //ctor                   Variable( UINT, D3D12ProgramBindDesc );

      // virtual fns begin
      void                   SetResource( ResourceHandle ) override;
      void                   SetResourceAtIndex( int, ResourceHandle ) override;
      void                   SetBindlessArray( IShaderBindlessArray* ) override;
      // virtual fns end

      StringView             GetName() const;
      //Span< DX12Descriptor > GetDescriptors( DX12TransitionHelper* ) const;

      RootParameterBinding*  GetRootParameterBinding() const;

    //private:
      //void                   SetArrayElement( int, IHandle );

      int            mRootParameterIndex;
      PipelineHandle mPipelineHandle;

      //RootParameterBinding*  mRootParameterBinding {};
    };

    struct Variables : public Vector< Variable >
    {
      Variables() = default;
      Variables( PipelineHandle, int );
      //Variables(const PipelineBindCache&);
      //Variables( const D3D12ProgramBindDescs& );
    };

    bool IsValid() const;

    PCom< ID3D12PipelineState > mPSO               {};
    PCom< ID3D12RootSignature > mRootSignature     {};
    Variables                   mShaderVariables   {};
    PipelineParams              mPipelineParams    {};
    PipelineBindCache           mPipelineBindCache {};
    bool                        mIsCompute         {};
  };
} // namespace Tac::Render

