#include "tac_gpu_input_layout.h"

namespace Tac::Render
{
  static_assert( GPUInputLayout::N >= ( int )Attribute::Count );
  static_assert( sizeof( GPUInputLayout ) == 16 * 3 * 4 + 4 );

  GPUInputLayout::GPUInputLayout( const VertexDeclarations& decls )
  {
    for( const VertexDeclaration& decl : decls )
    {
      const int i{ ( int )decl.mAttribute };

      TAC_ASSERT( i < N );
      mElementCounts[ i ] = ( uint )decl.mFormat.mElementCount;
      mGraphicsTypes[ i ] = ( uint )decl.mFormat.mPerElementDataType;
      mByteOffsets[ i ] = ( uint )decl.mAlignedByteOffset;
    }

    mStride = ( uint )decls.CalculateStride();
  }
} // namespace Tac
