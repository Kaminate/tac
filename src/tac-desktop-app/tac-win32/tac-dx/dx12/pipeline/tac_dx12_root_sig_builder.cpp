#include "tac_dx12_root_sig_builder.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"

namespace Tac::Render
{

  static D3D12_DESCRIPTOR_RANGE_TYPE
    D3D12ProgramBindingType_To_D3D12_DESCRIPTOR_RANGE_TYPE( const D3D12ProgramBindType type )
  {
    const D3D12ProgramBindType::Classification classification{ type.GetClassification() };
    switch( classification )
    {
    case D3D12ProgramBindType::kTextureUAV:
    case D3D12ProgramBindType::kBufferUAV:
      return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    case D3D12ProgramBindType::kTextureSRV:
    case D3D12ProgramBindType::kBufferSRV:
      return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    case D3D12ProgramBindType::kSampler:
      return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

    case D3D12ProgramBindType::kConstantBuffer:
      return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

    default:
      TAC_ASSERT_INVALID_CASE( classification );
      return ( D3D12_DESCRIPTOR_RANGE_TYPE )0;
    }
  }

  static D3D12_ROOT_PARAMETER_TYPE
    D3D12ProgramBindingType_To_D3D12_ROOT_PARAMETER_TYPE( const D3D12ProgramBindType type )
  {
    const D3D12ProgramBindType::Classification classification{ type.GetClassification() };
    switch( classification )
    {
    case D3D12ProgramBindType::kTextureUAV:
    case D3D12ProgramBindType::kBufferUAV:
      return D3D12_ROOT_PARAMETER_TYPE_UAV;

    case D3D12ProgramBindType::kTextureSRV:
    case D3D12ProgramBindType::kBufferSRV:
      return D3D12_ROOT_PARAMETER_TYPE_SRV;

    case D3D12ProgramBindType::kConstantBuffer:
      return D3D12_ROOT_PARAMETER_TYPE_CBV;

    default:
      TAC_ASSERT_INVALID_CASE( classification );
      return ( D3D12_ROOT_PARAMETER_TYPE )0;
    }
  }

  // -----------------------------------------------------------------------------------------------

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

  void DX12RootSigBuilder::SetInputLayoutEnabled( bool enabled )
  {
    mHasInputLayout = enabled;
  }

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

    // ????????????
    // read https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-version-1-1
    // maybe
    const D3D12_DESCRIPTOR_RANGE_FLAGS Flags
    {
      D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
      //D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
      //D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
    };

    *range = D3D12_DESCRIPTOR_RANGE1
    {
      .RangeType                         { type },
      .NumDescriptors                    { NumDescriptors },
      .BaseShaderRegister                { ( UINT )loc.mRegister },
      .RegisterSpace                     { ( UINT )loc.mSpace },
      .Flags                             { Flags },
      .OffsetInDescriptorsFromTableStart {},
    };

    const D3D12_ROOT_DESCRIPTOR_TABLE1 DescriptorTable
    {
      .NumDescriptorRanges { 1 },
      .pDescriptorRanges   {}, // fill this in later
    };

    const D3D12_ROOT_PARAMETER1 rootParam
    {
      .ParameterType    { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE },
      .DescriptorTable  { DescriptorTable },
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
      .ShaderRegister { ( UINT )loc.mRegister },
      .RegisterSpace  { ( UINT )loc.mSpace },
      .Flags          { D3D12_ROOT_DESCRIPTOR_FLAG_NONE },
    };

    const D3D12_ROOT_PARAMETER1 rootParam
    {
      .ParameterType    { type },
      .Descriptor       { Descriptor },
      .ShaderVisibility { D3D12_SHADER_VISIBILITY_ALL },
    };

    mRootParams.push_back( rootParam );
  }

  PCom< ID3D12RootSignature > DX12RootSigBuilder::Build( Errors& errors )
  {
    // root param is allowed to be empty
    //TAC_ASSERT( !mRootParams.empty() );

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
    D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags{ D3D12_ROOT_SIGNATURE_FLAG_NONE };
    if( mHasInputLayout )
      rootSigFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


    const D3D12_ROOT_SIGNATURE_DESC1 Desc_1_1
    {
      .NumParameters { ( UINT )mRootParams.size() },
      .pParameters   { mRootParams.data() },
      .Flags         { rootSigFlags },
    };

    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc
    {
      .Version  { D3D_ROOT_SIGNATURE_VERSION_1_1 },
      .Desc_1_1 { Desc_1_1 },
    };

    PCom<ID3DBlob> blob;
    PCom<ID3DBlob> blobErr;

    const HRESULT serializeHr{
      D3D12SerializeVersionedRootSignature( &desc,
                                            blob.CreateAddress(),
                                            blobErr.CreateAddress() ) };

    TAC_RAISE_ERROR_IF_RETURN(
      FAILED( serializeHr ),
      String() +
      "Failed to serialize root signature! "
      "Blob = " + ( const char* )blobErr->GetBufferPointer() + ", "
      "HRESULT = " + DX12_HRESULT_ToString( serializeHr )
    );


    PCom< ID3D12RootSignature > rootSignature;
    TAC_DX12_CALL_RET( mDevice->CreateRootSignature( 0,
                       blob->GetBufferPointer(),
                       blob->GetBufferSize(),
                       rootSignature.iid(),
                       rootSignature.ppv() ) );

    return rootSignature;
  }

  void DX12RootSigBuilder::AddBindings( const D3D12ProgramBindDesc* bindDescs, int n )
  {
    for( int i{}; i < n; ++i )
    {
      const D3D12ProgramBindDesc& bindDesc{ bindDescs[ i ] };

      const DX12RootSigBuilder::Location loc
      {
        .mRegister { bindDesc.mBindRegister },
        .mSpace    { bindDesc.mRegisterSpace },
      };

      if( bindDesc.BindsAsDescriptorTable() )
      {
        // Create a root descriptor table
        const D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType {
          D3D12ProgramBindingType_To_D3D12_DESCRIPTOR_RANGE_TYPE( bindDesc.mType ) };

        if( bindDesc.mBindCount )
          AddBoundedArray( descriptorRangeType, bindDesc.mBindCount, loc );
        else
          AddUnboundedArray( descriptorRangeType, loc );
      }
      else
      {
        // Create a root descriptor
        const D3D12_ROOT_PARAMETER_TYPE rootParameterType{
          D3D12ProgramBindingType_To_D3D12_ROOT_PARAMETER_TYPE( bindDesc.mType ) };
        AddRootDescriptor( rootParameterType, loc );
      }

    }
  }

} // namespace Tac::Render
