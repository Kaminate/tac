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
  struct IPipelineArray
  {
  };

  // A new DX12DescriptorRegion is allocated for each draw call
  struct PipelineDynamicArray : public IPipelineArray
  {
    
  };

  struct RootParameterBinding
  {
    // i feel like it makes sense for pipeline arrays to be dynamic by default.
    // instead of having a static array, just use a bindless array
    PipelineDynamicArray mPipelineDynamicArray {};
    IPipelineArray*      mPipelineArray        {};
    ResourceHandle       mResourceHandle       {};
    D3D12ProgramBindDesc mProgramBindDesc      {};
    UINT                 mRootParameterIndex   {};
  };

  struct PipelineBindCache : public Vector< RootParameterBinding > {};


  // bindless, DX12DescriptorRegion persists between draw calls
  struct PipelineBindlessArray : public IPipelineArray
  {
    struct Binding { int mIndex; };

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

