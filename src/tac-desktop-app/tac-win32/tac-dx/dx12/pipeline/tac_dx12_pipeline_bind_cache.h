// The purpose of this file is to keep track of what resources are
// bound to 
//
// 

#pragma once

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h" // DX12DescriptorRegion
#include "tac-dx/dx12/program/tac_dx12_program_bind_type.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{
  struct PipelineBindCache
  {
    struct Table
    {
      DX12DescriptorRegion mRegion;
    };

    Vector< Table > mRootTables;
  };

  struct PipelineBindlessArray
  {
    struct [[nodiscard]] Binding
    {
      ctor Binding() = default;
      dtor ~Binding();
      bool IsValid() const;
      void Unbind();

    private:
      Binding( int, PipelineBindlessArray* );
      friend struct PipelineBindlessArray;
      int                    mIndex { -1 };
      PipelineBindlessArray* mArray {};
    };

    Binding BindBuffer( BufferHandle );
    Binding BindTexture( TextureHandle );
    Binding BindSampler( SamplerHandle );

    void    Unbind( Binding );

  private:
    Binding BindInternal( int );

    D3D12ProgramBindType mType           {};
    Vector< int >        mHandleIndexes  {};
    Vector< Binding >    mUnusedBindings {};
  };

} // namespace Tac::Render

