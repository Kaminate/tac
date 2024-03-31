#include "tac_dx12_shader_program_mgr.h" // self-inc

#include "tac-win32/dx/hlsl/tac_hlsl_preprocess.h"
#include "tac-win32/dx/dxc/tac_dxc.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
#include "tac-win32/dx/dx12/tac_dx12_root_sig_bindings.h" // D3D12RootSigBindings
#include "tac-std-lib/filesystem/tac_filesystem.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{
  static const D3D_SHADER_MODEL sShaderModel = D3D_SHADER_MODEL_6_5;

  static D3D12_SHADER_BYTECODE IDxcBlobToBytecode( PCom<IDxcBlob>& blob )
  {
    return blob ?
      D3D12_SHADER_BYTECODE{ blob->GetBufferPointer(), blob->GetBufferSize() } :
      D3D12_SHADER_BYTECODE{};
  }

  static D3D_SHADER_MODEL GetHighestShaderModel( ID3D12Device* device )
  {
    const D3D_SHADER_MODEL lowestDefined = D3D_SHADER_MODEL_5_1;
    const D3D_SHADER_MODEL highestDefined = D3D_SHADER_MODEL_6_7; // D3D_HIGHEST_SHADER_MODEL undefined?;
    for( D3D_SHADER_MODEL shaderModel = highestDefined; 
         shaderModel >= lowestDefined;
         shaderModel = D3D_SHADER_MODEL( shaderModel - 1 ) )
    {
      // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_shader_model
      //   After the function completes successfully, the HighestShaderModel field contains the
      //   highest shader model that is both supported by the device and no higher than the
      //   shader model passed in.
      TAC_NOT_CONST D3D12_FEATURE_DATA_SHADER_MODEL featureData{ shaderModel };
      if( SUCCEEDED( device->CheckFeatureSupport(
        D3D12_FEATURE_SHADER_MODEL,
        &featureData,
        sizeof(D3D12_FEATURE_DATA_SHADER_MODEL) ) ) )
        
        // For some godforsaken fucking reason, this isn't the same as shaderModel
        return featureData.HighestShaderModel;
    }

    return lowestDefined;
  }

  static D3D12RootSigBinding::Type ShaderInputToRootSigBindType( D3D_SHADER_INPUT_TYPE Type )
  {
    switch( Type )
    {
    case D3D_SIT_CBUFFER: return D3D12RootSigBinding::Type::kCBuf;
    case D3D_SIT_TEXTURE: return D3D12RootSigBinding::Type::kTexture;
    case D3D_SIT_SAMPLER: return D3D12RootSigBinding::Type::kSampler;
    case D3D_SIT_BYTEADDRESS: return D3D12RootSigBinding::Type::kSRV;
    default: TAC_ASSERT_INVALID_CASE( Type ); return D3D12RootSigBinding::Type::kUnknown;
    }
  }

  static D3D12RootSigBinding       ShaderInputToRootSigBinding(D3D12_SHADER_INPUT_BIND_DESC& info)
  {
      D3D12RootSigBinding::Type type  = ShaderInputToRootSigBindType( info.Type );
      D3D12RootSigBinding binding
      {
        .mType = type,
        .mName = info.Name,
        .mCount = (int)info.BindCount,
      };
      return binding;
  }

  static D3D12_DESCRIPTOR_RANGE_TYPE ShaderInputToDescriptorRangeType( D3D_SHADER_INPUT_TYPE Type )
  {
    switch( Type )
    {
    case D3D_SIT_SAMPLER: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    case D3D_SIT_CBUFFER: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case D3D_SIT_BYTEADDRESS: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case D3D_SIT_TEXTURE: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    default: TAC_ASSERT_INVALID_CASE( Type ); return D3D12_DESCRIPTOR_RANGE_TYPE_SRV; 
    }
  }

  static D3D12_ROOT_PARAMETER_TYPE ShaderInputToRootParamType( D3D_SHADER_INPUT_TYPE Type )
  {
    switch( Type )
    {
    case D3D_SIT_CBUFFER: return D3D12_ROOT_PARAMETER_TYPE_CBV;
    case D3D_SIT_BYTEADDRESS: return D3D12_ROOT_PARAMETER_TYPE_SRV;
    case D3D_SIT_TEXTURE: return D3D12_ROOT_PARAMETER_TYPE_SRV;
    default: TAC_ASSERT_INVALID_CASE( Type ); return (D3D12_ROOT_PARAMETER_TYPE)0; 
    }
  }

  static D3D12_ROOT_PARAMETER1 ShaderInputToRootParam( D3D12_SHADER_INPUT_BIND_DESC& info,
                                                       FixedVector< D3D12_DESCRIPTOR_RANGE1, 100 >& mRanges )
  {
    if( info.BindCount )
    {
      D3D12_ROOT_PARAMETER_TYPE ParameterType = ShaderInputToRootParamType(info.Type);
      TAC_ASSERT( ParameterType != ( D3D12_ROOT_PARAMETER_TYPE )-1 );

      D3D12_ROOT_PARAMETER1 rootParam
      {
        .ParameterType = ParameterType,
        .Descriptor = D3D12_ROOT_DESCRIPTOR1
        {
          .ShaderRegister = info.BindPoint,
          .RegisterSpace = info.Space,
          .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE ,
        },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
      };
      return rootParam;
    }
    else
    {
      D3D12_DESCRIPTOR_RANGE_TYPE RangeType = ShaderInputToDescriptorRangeType( info.Type );
      D3D12_DESCRIPTOR_RANGE1* range = &mRanges.back();
      *range = D3D12_DESCRIPTOR_RANGE1{
        .RangeType = RangeType,
        .NumDescriptors = UINT_MAX, // unbounded
        .BaseShaderRegister = info.BindPoint,
        .RegisterSpace = info.Space,
        .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
        .OffsetInDescriptorsFromTableStart = 0,
      };
      mRanges.resize( mRanges.size() + 1 );

      D3D12_ROOT_PARAMETER1 rootParam
      {
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        .DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1
        {
          .NumDescriptorRanges = 1,
          .pDescriptorRanges = range,
        },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
      };

      return rootParam;
    }
  }

  // -----------------------------------------------------------------------------------------------

  void DX12ShaderProgramMgr::Init( ID3D12Device* device, Errors& errors )
  {
    mDevice = device;

    const D3D_SHADER_MODEL highestShaderModel = GetHighestShaderModel( device );
    TAC_RAISE_ERROR_IF( sShaderModel > highestShaderModel, "Shader model too high" );
  }

  DX12ShaderProgram* DX12ShaderProgramMgr::FindProgram( ProgramHandle h )
  {
    return h.IsValid() ? &mShaderPrograms[ h.GetIndex() ] : nullptr;
  }

  void DX12ShaderProgramMgr::CreateShaderProgram( ProgramHandle h,
                                                  ShaderProgramParams params,
                                                  Errors& errors)
  {
    const String fileName = params.mFileStem + ".hlsl";
    const String path = "assets/hlsl/" + fileName;
    const String preprocessedShader = TAC_CALL( HLSLPreprocess( path, errors ) );
    const Filesystem::Path outputDir = RenderApi::GetShaderOutputPath();
    const DXCCompileParams input
    {
      .mFileName = fileName,
      .mPreprocessedShader = preprocessedShader,
      .mShaderModel = sShaderModel,
      .mOutputDir = outputDir,
    };

    DXCCompileOutput output = TAC_CALL( DXCCompile( input, errors ) );


    Vector< D3D12_ROOT_PARAMETER1 > mRootParams;

    // This cannot be a Vector<> because D3D12_ROOT_PARAMETER1 may point to it
    FixedVector< D3D12_DESCRIPTOR_RANGE1, 100 > mRanges;

    D3D12RootSigBindings bindings;

    // Here's what im thinking.
    // Every descriptor table has its own root parameter,
    // so there wouldn't be a descriptor table that has CBVs, SRVs, and UAVs.
    // Instead that would be 3 separate root parameters.
    for( D3D12_SHADER_INPUT_BIND_DESC& info : output.mReflInfo.mReflBindings )
    {
      D3D12RootSigBinding binding = ShaderInputToRootSigBinding( info );
      bindings.mBindings.push_back( binding );

      D3D12_ROOT_PARAMETER1 rootParam = ShaderInputToRootParam( info, mRanges );
      mRootParams.push_back( rootParam  );
    }


    const D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc
    {
      .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
      .Desc_1_1 = D3D12_ROOT_SIGNATURE_DESC1
      {
        .NumParameters = ( UINT )mRootParams.size(),
        .pParameters = mRootParams.data(),
        .Flags = rootSigFlags,
      },
    };

    PCom< ID3DBlob > blob;
    PCom< ID3DBlob > blobErr;

    TAC_RAISE_ERROR_IF( const HRESULT hr =
                               D3D12SerializeVersionedRootSignature(
                               &desc,
                               blob.CreateAddress(),
                               blobErr.CreateAddress() ); FAILED( hr ),
                               String() +
                               "Failed to serialize root signature! "
                               "Blob = " + ( const char* )blobErr->GetBufferPointer() + ", "
                               "HRESULT = " + DX12_HRESULT_ToString( hr ) );

    PCom< ID3D12RootSignature > rootSignature;
    TAC_DX12_CALL( mDevice->CreateRootSignature( 0,
                       blob->GetBufferPointer(),
                       blob->GetBufferSize(),
                       rootSignature.iid(),
                       rootSignature.ppv() ) );

    mShaderPrograms[ h.GetIndex() ] = DX12ShaderProgram
    {
      .mFileStem = params.mFileStem,
      .mVSBlob = output.mVSBlob,
      .mVSBytecode = IDxcBlobToBytecode( output.mVSBlob ),
      .mPSBlob = output.mPSBlob,
      .mPSBytecode = IDxcBlobToBytecode( output.mPSBlob ),
      .mRootSignature = rootSignature,
      .mRootSignatureBindings = bindings,
    };

  }
} // namespace Tac::Render

