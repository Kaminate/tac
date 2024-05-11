#include "tac_dx12_pipeline.h" // self-inc
#include "tac-win32/dx/dx12/program/tac_dx12_program_bindings.h"

namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------

  DX12Pipeline::Variable::Variable( const D3D12ProgramBinding* binding )
  {
    mBinding = binding;
    mHandleIndexes.resize( binding->mBindCount, -1 );
  }

  void DX12Pipeline::Variable::SetBuffer( BufferHandle h )
  {
    TAC_ASSERT( mBinding->IsBuffer() );
    SetElement( h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetTexture( TextureHandle h )
  {
    TAC_ASSERT( mBinding->IsTexture() );
    SetElement( h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetBufferAtIndex( int i, BufferHandle h )
  {
    TAC_ASSERT( mBinding->IsBuffer() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetTextureAtIndex( int i, TextureHandle h )
  {
    TAC_ASSERT( mBinding->IsTexture() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetSamplerAtIndex( int i, SamplerHandle h )
  {
    TAC_ASSERT( mBinding->IsSampler() );
    SetArrayElement( i, h.GetIndex() );
  }

  void DX12Pipeline::Variable::SetElement( int iHandle )
  {
    TAC_ASSERT( mHandleIndexes.size() == 1 );
    TAC_ASSERT( mBinding->IsSingleElement() );
    mHandleIndexes[ 0 ] = iHandle;
  }

  void DX12Pipeline::Variable::SetArrayElement( int iArray, int iHandle )
  {
    TAC_ASSERT_INDEX( iArray, 1000 ); // sanity
    TAC_ASSERT( mBinding->IsFixedArray() || mBinding->IsUnboundedArray() );

    const int n{ mHandleIndexes.size() };
    if( const bool isUnboundedArray{ mBinding->mBindCount == 0 } )
    {
      if( !( iArray < n ) )
      {
        mHandleIndexes.resize( iArray + 1, -1 );
      }
    }
    else if( const bool isFixedArray{ mBinding->mBindCount > 1 } )
    {
      TAC_ASSERT_INDEX( iArray, n ); // sanity
    }

    mHandleIndexes[ iArray ] = iHandle;
  }


  StringView DX12Pipeline::Variable::GetName() const
  {
    return mBinding->mName;
  }
} // namespace Tac::Render

