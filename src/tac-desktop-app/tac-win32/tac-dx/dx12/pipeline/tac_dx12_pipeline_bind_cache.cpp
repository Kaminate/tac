#include "tac_dx12_pipeline_bind_cache.h" // self-inc

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  PipelineBindlessArray::Binding::Binding( int i ) : mIndex{ i } {}

  // -----------------------------------------------------------------------------------------------

  PipelineBindlessArray::Binding PipelineBindlessArray::BindBuffer( BufferHandle h )
  {
    TAC_ASSERT( mType.IsBuffer() );
    return BindInternal( h.GetIndex() );
  }

  PipelineBindlessArray::Binding PipelineBindlessArray::BindTexture( TextureHandle h )
  {
    TAC_ASSERT( mType.IsTexture() );
    return BindInternal( h.GetIndex() );
  }

  PipelineBindlessArray::Binding PipelineBindlessArray::BindSampler( SamplerHandle h )
  {
    TAC_ASSERT( mType.IsSampler() );
    return BindInternal( h.GetIndex() );
  }

  void                           PipelineBindlessArray::Unbind( Binding binding )
  {
    mUnusedBindings.push_back( binding );
    mHandleIndexes[ binding.mIndex ] = IHandle::kInvalidIndex;
  }

  PipelineBindlessArray::Binding PipelineBindlessArray::BindInternal(  int iHandle )
  {
    TAC_ASSERT_INDEX( iHandle, 1000 ); // sanity

    if( mUnusedBindings.empty() )
    {
      const int n{ mHandleIndexes.size() };
      const Binding binding{ n };
      mHandleIndexes.resize( n + 1, iHandle );
      return binding;
    }
    else
    {
      const Binding binding{ mUnusedBindings.back() };
      mUnusedBindings.pop_back();
      mHandleIndexes[ binding.mIndex ] = iHandle;
      return binding;
    }

  }

} // namespace Tac::Render

