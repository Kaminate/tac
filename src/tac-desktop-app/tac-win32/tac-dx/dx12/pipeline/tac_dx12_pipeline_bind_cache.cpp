#include "tac_dx12_pipeline_bind_cache.h" // self-inc

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------
  //   PipelineArray::Binding
  // -----------------------------------------------------------------------------------------------

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
      mArray = nullptr;
    }
  }

  bool PipelineArray::Binding::IsValid() const
  {
    return mIndex != -1;
  }

  // -----------------------------------------------------------------------------------------------
  //                     PipelineArray
  // -----------------------------------------------------------------------------------------------

  PipelineArray::Binding PipelineArray::Bind( ResourceHandle h )
  {
    if constexpr ( kIsDebugMode )
    {
      const D3D12ProgramBindType type{ mProgramBindDesc.mType };
      TAC_ASSERT( type.IsValid() );
      TAC_ASSERT( !type.IsBuffer() || h.IsBuffer() );
      TAC_ASSERT( !type.IsTexture() || h.IsTexture() );
      TAC_ASSERT( !type.IsSampler() || h.IsSampler() );
    }

    const int iHandle{ h.GetIndex() };
    TAC_ASSERT_INDEX( iHandle, 1000 ); // sanity

    if( mUnusedBindings.empty() )
    {
      const int n{ mHandles.size() };
      const Binding binding( n, this );
      mHandles.resize( n + 1, iHandle );
      return binding;
    }
    else
    {
      const Binding binding{ mUnusedBindings.back() };
      mUnusedBindings.pop_back();
      mHandles[ binding.mIndex ] = h;
      return binding;
    }
  }

  void                   PipelineArray::Unbind( Binding binding )
  {
    mUnusedBindings.push_back( binding );
    mHandles[ binding.mIndex ] = IHandle::kInvalidIndex;
  }

  PipelineArray::Binding PipelineArray::BindInternal(  int iHandle )
  {
    TAC_ASSERT_UNIMPLEMENTED;
    return {};

  }

} // namespace Tac::Render

