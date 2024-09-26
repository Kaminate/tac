#include "tac_dx12_pipeline_bind_cache.h" // self-inc

namespace Tac::Render
{
  void PipelineBindlessArray::SetBufferAtIndex( int i, BufferHandle h)
  {
    TAC_ASSERT( mType.IsBuffer() );
    SetArrayElement( i, h.GetIndex() );
  }

  void PipelineBindlessArray::SetTextureAtIndex( int i, TextureHandle h )
  {
    TAC_ASSERT( mType.IsTexture() );
    SetArrayElement( i, h.GetIndex() );
  }

  void PipelineBindlessArray::SetSamplerAtIndex( int i, SamplerHandle h )
  {
    TAC_ASSERT( mType.IsSampler() );
    SetArrayElement( i, h.GetIndex() );
  }

  void PipelineBindlessArray::SetArrayElement( int i, int iHandle )
  {
    TAC_ASSERT_INDEX( i, 1000 ); // sanity

    // resize unbounded array
    if( i>= mHandleIndexes.size() )
      mHandleIndexes.resize( i + 1, -1 );

    mHandleIndexes[ i ] = iHandle;
  }

}

