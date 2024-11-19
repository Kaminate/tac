// The purpose of this file is to keep track of what resources are
// bound to 
//
// 

#pragma once

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h" // DX12DescriptorRegion
//#include "tac-dx/dx12/program/tac_dx12_program_bind_type.h"
#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{
  //struct IPipelineArray 
  //{
  //  virtual Span< DX12Descriptor > GetDescriptors( DX12TransitionHelper* ) const = 0;
  //};

  struct CommitParams
  {
    ID3D12GraphicsCommandList* mCommandList        {};
#if 0
    DX12DescriptorCaches*      mDescriptorCaches   {};
#endif
    bool                       mIsCompute          {};
    UINT                       mRootParameterIndex {};
  };

  // A new DX12DescriptorRegion is allocated for each draw call
  struct PipelineDynamicArray // : public IPipelineArray
  {
    Span< DX12Descriptor > GetDescriptors( DX12TransitionHelper* ) const; // override;
    void                   BindAtIndex( ResourceHandle, int );
    void                   SetFence( FenceSignal );
    void                   Commit( CommitParams );

  // private:
    void                   CheckType( ResourceHandle );
    DX12Descriptor         GetDescriptor( IHandle, DX12TransitionHelper* ) const;

    Vector< IHandle >      mHandleIndexes        {};
    D3D12ProgramBindType   mProgramBindType      {};

    // Temporary per draw call, allocated during Commit(), and freed during SetFence()
    DX12DescriptorRegion   mDescriptorRegion     {};

    int                    mMaxBoundIndex        { -1 };
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
    //Span< DX12Descriptor > GetDescriptors( DX12TransitionHelper* ) const;
    void                   SetFence( FenceSignal );

    void                   Commit( CommitParams );

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

  // bindless, DX12DescriptorRegion persists between draw calls
  // ( as oppposed to a PipelineDynamicArray, where a DX12DescriptorRegion is allocated per call )
  struct BindlessArray : public IBindlessArray
  {
    BindlessArray( IBindlessArray::Params );
    Binding                Bind( ResourceHandle ) override;
    void                   Unbind( Binding ) override;
    void                   Resize( int );
    void                   SetFenceSignal( FenceSignal );
    //Span< DX12Descriptor > GetDescriptors( DX12TransitionHelper* ) const override;

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

