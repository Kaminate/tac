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

  const VertexDeclaration* FindVtxDecl( const VertexDeclarations& vtxDecls,
                                        StringView semanticName )
  {
    for( const VertexDeclaration& vtxDecl : vtxDecls )
      if( ( StringView )GetSemanticName( vtxDecl.mAttribute ) == semanticName )
        return &vtxDecl;

    return nullptr;
  }

  // -----------------------------------------------------------------------------------------------

  // the vtxDecls must be >= program inputs
  DX12InputLayout::DX12InputLayout( const VertexDeclarations& vtxDecls,
                                    const DX12Program* program ) : D3D12_INPUT_LAYOUT_DESC{}
  {
    if( program->mInputs.empty() )
      return;

    for( const DX12Program::Input& programInput : program->mInputs )
    {
      const VertexDeclaration* vtxDecl{ FindVtxDecl( vtxDecls, programInput.mName ) };
      TAC_ASSERT( vtxDecl );
      TAC_ASSERT( programInput.mIndex == 0 );

      const D3D12_INPUT_ELEMENT_DESC inputElementDesc
      {
        .SemanticName         { GetSemanticName( vtxDecl->mAttribute ) },
        .SemanticIndex        {},
        .Format               { DXGIFormatFromVertexAttributeFormat( vtxDecl->mFormat ) },
        .InputSlot            {}, // which vertex buffer index this data comes from
        .AlignedByteOffset    { ( UINT )vtxDecl->mAlignedByteOffset },
        .InputSlotClass       { D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA },
        .InstanceDataStepRate {},
      };

      InputElementDescs.push_back( inputElementDesc );
    }


    pInputElementDescs = InputElementDescs.data();
    NumElements = ( UINT )InputElementDescs.size();
  }



} // namespace Tac::Render

