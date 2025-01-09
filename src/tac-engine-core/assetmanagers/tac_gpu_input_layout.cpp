#include "tac_gpu_input_layout.h"

namespace Tac::Render
{
  static_assert( sizeof( GPUInputLayout ) == 16 * 3 * 4 + 4 );

  GPUInputLayout::GPUInputLayout( const VertexDeclarations& decls )
  {
    for( const VertexDeclaration& decl : decls )
    {
      const int i{ ( int )decl.mAttribute };

      TAC_ASSERT( i < N );
      mElementCounts[ i ] = ( u32 )decl.mFormat.mElementCount;
      mGraphicsTypes[ i ] = ( u32 )decl.mFormat.mPerElementDataType;
      mByteOffsets[ i ] = ( u32 )decl.mAlignedByteOffset;
    }

    mStride = ( u32 )decls.CalculateStride();
  }
} // namespace Tac
