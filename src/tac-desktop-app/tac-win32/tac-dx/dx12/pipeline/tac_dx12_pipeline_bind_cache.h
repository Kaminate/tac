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
    virtual Span<DX12Descriptor> GetDescriptors( DX12TransitionHelper* ) = 0;
  };

  // A new DX12DescriptorRegion is allocated for each draw call
  struct PipelineDynamicArray : public IPipelineArray
  {
    Span<DX12Descriptor> GetDescriptors( DX12TransitionHelper* ) override;

  private:
    DX12Descriptor GetDescriptor( IHandle, DX12TransitionHelper* ) ;
    D3D12ProgramBindDesc mProgramBindDesc      {};
  };


  struct RootParameterBinding
  {
    enum class Type
    {
      kUnknown = 0,
      kDynamicArray,
      kBindlessArray,
      kResourceHandle,
    };
    Span<DX12Descriptor> GetDescriptors( DX12TransitionHelper* );

    // Resources can be bound in an array, or as a handle, depending
    // on the resource type
    PipelineDynamicArray mPipelineDynamicArray {};
    IPipelineArray*      mPipelineArray        {};
    ResourceHandle       mResourceHandle       {};

    D3D12ProgramBindDesc mProgramBindDesc      {};
    UINT                 mRootParameterIndex   {};
    Type                 mType                 {};
  };

  struct PipelineBindCache : public Vector< RootParameterBinding > {};


  // bindless, DX12DescriptorRegion persists between draw calls
  struct PipelineBindlessArray : public IPipelineArray , public IShaderBindlessArray // ???
  {
    struct Binding { int mIndex; };

    Binding Bind( ResourceHandle );
    Binding BindAtIndex( ResourceHandle, int );
    void    Unbind( Binding );
    void    Resize( int );
    void    SetFenceSignal( FenceSignal );
    Span<DX12Descriptor> GetDescriptors( DX12TransitionHelper* ) override;

  private:

    using HeapType = D3D12_DESCRIPTOR_HEAP_TYPE;

    HeapType GetHeapType() const;
    void     CheckType( ResourceHandle );

    D3D12ProgramBindType mProgramBindType  {};
    Vector< IHandle >    mHandles          {};
    Vector< Binding >    mUnusedBindings   {};
    DX12DescriptorRegion mDescriptorRegion {};
    FenceSignal          mFenceSignal      {};
    D3D12ProgramBindDesc mProgramBindDesc  {};
  };

} // namespace Tac::Render

