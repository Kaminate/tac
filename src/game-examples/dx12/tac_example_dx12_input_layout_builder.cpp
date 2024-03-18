#include "tac_example_dx12_input_layout_builder.h" // self-inc

#include "tac-win32/dx/dxgi/tac_dxgi.h"

namespace Tac::Render
{
    DX12BuiltInputLayout::DX12BuiltInputLayout( const VertexDeclarations& vtxDecls )
    {
      const int n = vtxDecls.size();
      mElementDescs.resize(n );
      for( int i = 0; i < n; ++i )
      {
        const auto& decl = vtxDecls[ i ];
        mElementDescs[ i ] = D3D12_INPUT_ELEMENT_DESC
        {
          .SemanticName = GetSemanticName( decl.mAttribute ),
          .Format = GetDXGIFormatTexture( decl.mTextureFormat ),
          .AlignedByteOffset = (UINT)decl.mAlignedByteOffset,
        };
      }

      *( D3D12_INPUT_LAYOUT_DESC* )this = D3D12_INPUT_LAYOUT_DESC
      {
        .pInputElementDescs = mElementDescs.data(),
        .NumElements = (UINT)n,
      };
    }
}// namespace
