#include "tac_dx12_input_layout.h" // self-inc

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-dx/dx12/program/tac_dx12_program.h"
#include "tac-dx/dxgi/tac_dxgi.h"


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

  DX12InputLayout::DX12InputLayout( const VertexDeclarations& vtxDecls,
                                    const DX12Program* program )
  {
    const int vertexDeclCount { vtxDecls.size() };
    TAC_ASSERT( vertexDeclCount == program->mInputs.size() );

    InputElementDescs.resize( vertexDeclCount );

    for( int i { 0 }; i < vertexDeclCount; ++i )
    {
      const DX12Program::Input& programInput{ program->mInputs[ i ] };
      const VertexDeclaration& vtxDecl { vtxDecls[ i ] };

      const char* semanticName{ GetSemanticName( vtxDecl.mAttribute ) };
      DXGI_FORMAT dxgiFmt{ GetDXGIFormatTexture( vtxDecl.mFormat ) };

      TAC_ASSERT( programInput.mName == semanticName );
      TAC_ASSERT( programInput.mIndex == 0 );

      const D3D12_INPUT_ELEMENT_DESC inputElementDesc
      {
        .SemanticName         { semanticName },
        .SemanticIndex        { 0 },
        .Format               { dxgiFmt },
        .InputSlot            {}, // which vertex buffer index this data comes from
        .AlignedByteOffset    { (UINT)vtxDecl.mAlignedByteOffset },
        .InputSlotClass       { D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA },
        .InstanceDataStepRate {},
      };

      InputElementDescs[ i ] = inputElementDesc;
    }

    *( ( D3D12_INPUT_LAYOUT_DESC* )this ) = D3D12_INPUT_LAYOUT_DESC
    {
      // the ternary suppress a d3d12 validation warning
      .pInputElementDescs { InputElementDescs.empty() ? nullptr : InputElementDescs.data() },

      .NumElements        { ( UINT )InputElementDescs.size() },
    };
  }



} // namespace Tac::Render

