#include "tac_dx12_pipeline_bind_cache.h" // self-inc

#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------
  //   PipelineArray::Binding
  // -----------------------------------------------------------------------------------------------

#if 0
  ctor PipelineArray::Binding::Binding( int i, PipelineArray* pba )
    : mIndex{ i }
    , mArray{ pba }
  {}

  dtor PipelineArray::Binding::~Binding()
  {
    Unbind();
  }

  void PipelineArray::Binding::Unbind()
  {
    if( IsValid() )
    {
      mArray->Unbind( *this );
      mIndex = -1;
      mArray = {};
    }
  }

  bool PipelineArray::Binding::IsValid() const
  {
    return mIndex != -1;
  }
#endif

  // -----------------------------------------------------------------------------------------------
  //                     PipelineArray
  // -----------------------------------------------------------------------------------------------

  void                    PipelineArray::CheckType( ResourceHandle h )
  {
    if constexpr ( !kIsDebugMode )
      return;

    const D3D12ProgramBindType type{ mProgramBindType };
    TAC_ASSERT( type.IsValid() );
    TAC_ASSERT( !type.IsBuffer() || h.IsBuffer() );
    TAC_ASSERT( !type.IsTexture() || h.IsTexture() );
    TAC_ASSERT( !type.IsSampler() || h.IsSampler() );
  }

  PipelineArray::HeapType PipelineArray::GetHeapType() const
  {
    const D3D12ProgramBindType type{ mProgramBindType };

    if( type.IsBuffer() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    if( type.IsTexture() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    if( type.IsSampler() )
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    TAC_ASSERT_INVALID_CODE_PATH;
    return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
  }

  void                    PipelineArray::SetFenceSignal( FenceSignal fenceSignal )
  {
    mFenceSignal = fenceSignal;
  }

  void                    PipelineArray::Resize( const int newSize )
  {
    const int oldSize{ mHandles.size() };

    TAC_ASSERT( mUnusedBindings.empty() );
    TAC_ASSERT( newSize > oldSize );

    mHandles.resize( newSize );
    for( int i{ oldSize }; i < newSize; ++i )
      mUnusedBindings.push_back( Binding{ i } );

    const HeapType heapType{ GetHeapType() };
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12DescriptorHeap& heap{ renderer.mGpuDescriptorHeaps[ heapType ] };
    DX12DescriptorAllocator* regionMgr{ heap.GetRegionMgr() };


    DX12DescriptorRegion newRegion{ regionMgr->Alloc( newSize ) };

    ID3D12Device* device{ renderer.mDevice };
    device->CopyDescriptorsSimple( oldSize,
                                   newRegion.GetCPUHandle(),
                                   mDescriptorRegion.GetCPUHandle(),
                                   heapType );

    mDescriptorRegion.Free( mFenceSignal );

    mDescriptorRegion = ( DX12DescriptorRegion&& )newRegion;
  }

  PipelineArray::Binding  PipelineArray::Bind( ResourceHandle h )
  {
    CheckType( h );

    if( mUnusedBindings.empty() )
    {
      const int n{ mHandles.size() };
      const Binding binding{ n };
      const int newSize{ ( int )( ( n + 2 ) * 1.5 ) };
      mHandles.resize( newSize );
    }

    const Binding binding{ mUnusedBindings.back() };
    mUnusedBindings.pop_back();
    mHandles[ binding.mIndex ] = h;

    {
    }

    return binding;
  }

  PipelineArray::Binding  PipelineArray::BindAtIndex( ResourceHandle h, int i )
  {
    CheckType( h );

    bool bindReady{};
    for( Binding& binding : mUnusedBindings )
    {
      if( binding.mIndex == i )
      {
        Swap( binding, mUnusedBindings.back() );
        mUnusedBindings.pop_back();
        bindReady = true;
      }
    }

    TAC_ASSERT( bindReady );
    mHandles[ i ] = h;

    return Binding{ i };
  }

  void                    PipelineArray::Unbind( Binding binding )
  {
    mUnusedBindings.push_back( binding );
    mHandles[ binding.mIndex ] = IHandle::kInvalidIndex;
  }


} // namespace Tac::Render

