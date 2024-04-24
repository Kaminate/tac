#include "tac_dx12_root_sig_builder.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"

namespace Tac::Render
{
  DX12RootSigBuilder::DX12RootSigBuilder( ID3D12Device* device ) : mDevice( device ) {}
#if 0

  void DX12RootSigBuilder::AddRootDescriptorTable( D3D12_SHADER_VISIBILITY vis,
                                                   D3D12_DESCRIPTOR_RANGE1 toAdd )
  {
    AddRootDescriptorTable( vis, Span( toAdd ) );
  }

  void DX12RootSigBuilder::AddConstantBuffer( Location loc )
  {
    const D3D12_ROOT_DESCRIPTOR1 desc
    {
      .ShaderRegister { ( UINT )loc.mRegister },
      .RegisterSpace  { ( UINT )loc.mSpace },
      .Flags          { D3D12_ROOT_DESCRIPTOR_FLAG_NONE },
    };

    const D3D12_ROOT_PARAMETER1 rootParam
    {
      .ParameterType { D3D12_ROOT_PARAMETER_TYPE_CBV },
      .Descriptor { desc },
      .ShaderVisibility { D3D12_SHADER_VISIBILITY_ALL },
    };

    mRootParams.push_back( rootParam );
  }
#endif

  D3D12_DESCRIPTOR_RANGE1* DX12RootSigBuilder::AddRange( int n )
  {
    const int rangeOffset { mRanges.size() };
    mRangeOffsets.push_back( rangeOffset );
    mRanges.resize( mRanges.size() + n );
    return &mRanges[ rangeOffset ];
  }

  void DX12RootSigBuilder::AddBoundedArray( D3D12_DESCRIPTOR_RANGE_TYPE type, int n, Location loc )
  {
    AddArrayInternal( type, n, loc );
  }

  void DX12RootSigBuilder::AddArrayInternal( D3D12_DESCRIPTOR_RANGE_TYPE type,
                                             UINT NumDescriptors,
                                             Location loc )
  {
    D3D12_DESCRIPTOR_RANGE1* range { AddRange() };
    *range = D3D12_DESCRIPTOR_RANGE1
    {
      .RangeType                         { type },
      .NumDescriptors                    { NumDescriptors },
      .BaseShaderRegister                { ( UINT )loc.mRegister },
      .RegisterSpace                     { ( UINT )loc.mSpace },
      .Flags                             { D3D12_DESCRIPTOR_RANGE_FLAG_NONE },
      .OffsetInDescriptorsFromTableStart { 0 },
    };

    const D3D12_ROOT_DESCRIPTOR_TABLE1 DescriptorTable
    {
      .NumDescriptorRanges { 1 },
      .pDescriptorRanges { range },
    };

    const D3D12_ROOT_PARAMETER1 rootParam
    {
      .ParameterType { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE },
      .DescriptorTable { DescriptorTable },
      .ShaderVisibility { D3D12_SHADER_VISIBILITY_ALL },
    };

    mRootParams.push_back( rootParam );
  }
  void DX12RootSigBuilder::AddUnboundedArray( D3D12_DESCRIPTOR_RANGE_TYPE type, Location loc )
  {
    AddArrayInternal( type, UINT_MAX, loc );
  }

  void DX12RootSigBuilder::AddRootDescriptor( D3D12_ROOT_PARAMETER_TYPE type, Location loc)
  {
    const D3D12_ROOT_DESCRIPTOR1 Descriptor
    {
      .ShaderRegister{ ( UINT )loc.mRegister },
      .RegisterSpace { ( UINT )loc.mSpace },
      .Flags         { D3D12_ROOT_DESCRIPTOR_FLAG_NONE },
    };

    const D3D12_ROOT_PARAMETER1 rootParam
    {
      .ParameterType    { type },
      .Descriptor       { Descriptor },
      .ShaderVisibility { D3D12_SHADER_VISIBILITY_ALL },
    };

    mRootParams.push_back( rootParam );
  }

#if 0
  void DX12RootSigBuilder::AddRootDescriptor( D3D12_ROOT_PARAMETER_TYPE paramType,
                                              D3D12_SHADER_VISIBILITY vis,
                                              D3D12_ROOT_DESCRIPTOR1 desc )
  {
    const D3D12_ROOT_PARAMETER1 rootParam
    {
      .ParameterType { paramType },
      .Descriptor { desc },
      .ShaderVisibility { vis },
    };

    mRootParams.push_back( rootParam );
  }


  void DX12RootSigBuilder::AddConstantBuffer( D3D12_SHADER_VISIBILITY vis,
                                              D3D12_ROOT_DESCRIPTOR1 descriptor )
  {
    const D3D12_ROOT_PARAMETER1 rootParam
    {
      .ParameterType { D3D12_ROOT_PARAMETER_TYPE_CBV },
      .Descriptor { descriptor },
      .ShaderVisibility { vis },
    };

    mRootParams.push_back( rootParam );
  }

  void DX12RootSigBuilder::AddRootDescriptorTable( D3D12_SHADER_VISIBILITY vis,
                                                   Span<D3D12_DESCRIPTOR_RANGE1> toAdd )
  {
    Span dst( &mRanges[ mRanges.size() ], toAdd.size() );
    dst = toAdd;
    mRanges.resize( mRanges.size() + toAdd.size() );

    const D3D12_ROOT_PARAMETER1 rootParam
    {
      .ParameterType { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE },
      .DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1
      {
        .NumDescriptorRanges { ( UINT )dst.size() },
        .pDescriptorRanges { dst.data() },
      },
      .ShaderVisibility { vis },
    };

    mRootParams.push_back( rootParam );
  }
#endif

  PCom< ID3D12RootSignature > DX12RootSigBuilder::Build( Errors& errors )
  {
    TAC_ASSERT( !mRootParams.empty() );

    int* rangeOffset { mRangeOffsets.data() };
    for( D3D12_ROOT_PARAMETER1& rootParam : mRootParams )
      if( rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE )
        rootParam.DescriptorTable.pDescriptorRanges = mRanges.data() + *rangeOffset++;

    // D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    //
    //   Omitting this flag can result in one root argument space being saved on some hardware.
    //   Omit this flag if the Input Assembler is not required, though the optimization is minor.
    //   This flat opts in to using the input assembler, which requires an input layout that
    //   defines a set of vertex buffer bindings.
    const D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    const D3D12_ROOT_SIGNATURE_DESC1 Desc_1_1
    {
      .NumParameters { ( UINT )mRootParams.size() },
      .pParameters   { mRootParams.data() },
      .Flags         { rootSigFlags },
    },

    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc
    {
      .Version  { D3D_ROOT_SIGNATURE_VERSION_1_1 },
      .Desc_1_1 { Desc_1_1 },
    };

    PCom<ID3DBlob> blob;
    PCom<ID3DBlob> blobErr;

    TAC_RAISE_ERROR_IF_RETURN( const HRESULT hr =
                               D3D12SerializeVersionedRootSignature(
                               &desc,
                               blob.CreateAddress(),
                               blobErr.CreateAddress() ); FAILED( hr ),
                               String() +
                               "Failed to serialize root signature! "
                               "Blob = " + ( const char* )blobErr->GetBufferPointer() + ", "
                               "HRESULT = " + DX12_HRESULT_ToString( hr ), {} );

    PCom< ID3D12RootSignature > rootSignature;
    TAC_DX12_CALL_RET( {},
                       mDevice->CreateRootSignature( 0,
                       blob->GetBufferPointer(),
                       blob->GetBufferSize(),
                       rootSignature.iid(),
                       rootSignature.ppv() ) );

    return rootSignature;
  }


} // namespace Tac::Render
