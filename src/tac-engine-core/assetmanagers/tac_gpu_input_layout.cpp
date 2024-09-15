#include "tac_gpu_input_layout.h"


namespace Tac::Render
{
  GPUInputLayout::GPUInputLayout( const VertexDeclarations& decls )
  {
    for( const VertexDeclaration& decl : decls )
    {
      const int i{ ( int )decl.mAttribute };

      mElementCounts[ i ] = ( u8 )decl.mFormat.mElementCount;
      mGraphicsTypes[ i ] = ( u8 )decl.mFormat.mPerElementDataType;
      mByteOfffsets[ i ] = ( u8 )decl.mAlignedByteOffset;
    }

    mStride = decls.CalculateStride();
  }
} // namespace Tac
