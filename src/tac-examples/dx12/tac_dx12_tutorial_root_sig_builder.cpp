#include "tac_dx12_tutorial_root_sig_builder.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-dx/dx12/tac_dx12_helper.h"

namespace Tac::Render
{
    DX12ExampleRootSignatureBuilder::DX12ExampleRootSignatureBuilder( ID3D12Device* device ) : mDevice( device ) {}

    void DX12ExampleRootSignatureBuilder::AddRootDescriptorTable( D3D12_SHADER_VISIBILITY vis,
                                                                  D3D12_DESCRIPTOR_RANGE1 toAdd )
    {
      AddRootDescriptorTable( vis, Span( toAdd ) );
    }

    void DX12ExampleRootSignatureBuilder::AddRootDescriptor( D3D12_ROOT_PARAMETER_TYPE paramType,
                                                             D3D12_SHADER_VISIBILITY vis,
                                                             D3D12_ROOT_DESCRIPTOR1 desc )
    {
      const D3D12_ROOT_PARAMETER1 rootParam
      {
        .ParameterType = paramType,
        .Descriptor = desc,
        .ShaderVisibility = vis,
      };

      mRootParams.push_back( rootParam );
    }


    void DX12ExampleRootSignatureBuilder::AddConstantBuffer( D3D12_SHADER_VISIBILITY vis,
                                                             D3D12_ROOT_DESCRIPTOR1 descriptor )
    {
      const D3D12_ROOT_PARAMETER1 rootParam
      {
        .ParameterType    { D3D12_ROOT_PARAMETER_TYPE_CBV },
        .Descriptor       { descriptor },
        .ShaderVisibility { vis },
      };

      mRootParams.push_back( rootParam );
    }

    void DX12ExampleRootSignatureBuilder::AddRootDescriptorTable( D3D12_SHADER_VISIBILITY vis,
                                                                  Span<D3D12_DESCRIPTOR_RANGE1> toAdd )
    {
      Span dst( &mRanges[ mRanges.size() ], toAdd.size() );
      dst = toAdd;
      mRanges.resize( mRanges.size() + toAdd.size() );

      const D3D12_ROOT_DESCRIPTOR_TABLE1 DescriptorTable
      {
        .NumDescriptorRanges { ( UINT )dst.size() },
        .pDescriptorRanges   { dst.data() },
      };

      const D3D12_ROOT_PARAMETER1 rootParam
      {
        .ParameterType    { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE },
        .DescriptorTable  { DescriptorTable },
        .ShaderVisibility { vis },
      };

      mRootParams.push_back( rootParam );
    }

    PCom< ID3D12RootSignature > DX12ExampleRootSignatureBuilder::Build( Errors& errors )
    {
      TAC_ASSERT( !mRootParams.empty() );

      // D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
      //
      //   Omitting this flag can result in one root argument space being saved on some hardware.
      //   Omit this flag if the Input Assembler is not required, though the optimization is minor.
      //   This flat opts in to using the input assembler, which requires an input layout that
      //   defines a set of vertex buffer bindings.
      const D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags { D3D12_ROOT_SIGNATURE_FLAG_NONE };

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

      PCom< ID3DBlob > blob;
      PCom< ID3DBlob > blobErr;


      const HRESULT serializeHr{
        D3D12SerializeVersionedRootSignature( &desc,
                                              blob.CreateAddress(),
                                              blobErr.CreateAddress() ) };
                                 

      TAC_RAISE_ERROR_IF_RETURN( {},
                                   FAILED( serializeHr ),
                                   String() +
                                   "Failed to serialize root signature! "
                                   "Blob = " + ( const char* )blobErr->GetBufferPointer() + ", "
                                   "HRESULT = " + DX12_HRESULT_ToString( serializeHr ) );

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
