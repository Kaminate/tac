// The purpose of this file is to keep track of what resources are
// bound to 
//
// 

#pragma once

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h" // DX12DescriptorRegion
//#include "tac-dx/dx12/program/tac_dx12_program_bind_type.h"
#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{
  struct PipelineArray;

  struct PipelineBindCache
  {
    struct Table
    {
      PipelineArray* mPipelineArray;
      bool           mPipelineArrayOwned;
    };

    Vector< Table > mRootTables;
  };

  struct PipelineArray
  {
    struct [[nodiscard]] Binding
    {
      ctor Binding() = default;
      dtor ~Binding();
      bool IsValid() const;
      void Unbind();

    private:
      Binding( int, PipelineArray* );
      friend struct PipelineArray;
      int            mIndex { -1 };
      PipelineArray* mArray {};
    };

    Binding Bind( ResourceHandle );
    void    Unbind( Binding );

    //bool    IsBufferArray() const;
    //bool    IsTextureArray() const;
    //bool    IsSamplerArray() const;

  private:
    Binding BindInternal( int );

    //D3D12ProgramBindType mType             {};
    D3D12ProgramBindDesc mProgramBindDesc {};
    Vector< IHandle >    mHandles;
    Vector< Binding >    mUnusedBindings   {};
    DX12DescriptorRegion mDescriptorRegion {};
  };

} // namespace Tac::Render

