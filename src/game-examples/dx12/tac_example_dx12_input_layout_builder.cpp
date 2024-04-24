#include "tac_example_dx12_input_layout_builder.h" // self-inc

#include "tac-win32/dx/dxgi/tac_dxgi.h"

namespace Tac::Render
{
  static const char* GetSemanticName( Attribute attribType )
  {
    switch( attribType )
    {
    case Attribute::Position:   return "POSITION";
    case Attribute::Normal:     return "NORMAL";
    case Attribute::Texcoord:   return "TEXCOORD";
    case Attribute::Color:      return "COLOR";
    case Attribute::BoneIndex:  return "BONEINDEX";
    case Attribute::BoneWeight: return "BONEWEIGHT";
    case Attribute::Coeffs:     return "COEFFS";
    default: TAC_ASSERT_INVALID_CASE( attribType ); return nullptr;
    }
  }

  DX12BuiltInputLayout::DX12BuiltInputLayout( const VertexDeclarations& vtxDecls )
  {
    const int n = vtxDecls.size();
    mElementDescs.resize( n );
    for( int i{}; i < n; ++i )
    {
      const auto& decl = vtxDecls[ i ];
      mElementDescs[ i ] = D3D12_INPUT_ELEMENT_DESC
      {
        .SemanticName = GetSemanticName( decl.mAttribute ),
        .Format = GetDXGIFormatTexture( decl.mTextureFormat ),
        .AlignedByteOffset = ( UINT )decl.mAlignedByteOffset,
      };
    }

    *( D3D12_INPUT_LAYOUT_DESC* )this = D3D12_INPUT_LAYOUT_DESC
    {
      .pInputElementDescs = mElementDescs.data(),
      .NumElements = ( UINT )n,
    };
  }
}// namespace
