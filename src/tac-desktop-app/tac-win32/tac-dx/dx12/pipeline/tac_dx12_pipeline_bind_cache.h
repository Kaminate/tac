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
    struct RootParameter
    {
      PipelineArray*       mPipelineArray      {};
      bool                 mPipelineArrayOwned {};
      ResourceHandle       mResourceHandle     {};
      D3D12ProgramBindDesc mProgramBindDesc    {};
      UINT                 mRootParameterIndex {};
    };

    Vector< RootParameter > mRootParameters;
  };

  struct PipelineArray
  {
#if 0
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
#else
    struct Binding
    {
      int mIndex;
    };
#endif

    Binding Bind( ResourceHandle );
    Binding BindAtIndex( ResourceHandle, int );
    void    Unbind( Binding );
    void    Resize( int );
    void    SetFenceSignal( FenceSignal );

  private:

    using HeapType = D3D12_DESCRIPTOR_HEAP_TYPE;

    HeapType GetHeapType() const;
    void     CheckType( ResourceHandle );

    D3D12ProgramBindType mProgramBindType  {};
    Vector< IHandle >    mHandles          {};
    Vector< Binding >    mUnusedBindings   {};
    DX12DescriptorRegion mDescriptorRegion {};
    FenceSignal          mFenceSignal      {};
  };

} // namespace Tac::Render

