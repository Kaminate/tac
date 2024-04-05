#include "tac_dx12_shader_program_mgr.h" // self-inc

#include "tac-win32/dx/hlsl/tac_hlsl_preprocess.h"
#include "tac-win32/dx/dxc/tac_dxc.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL
#include "tac-win32/dx/dx12/tac_dx12_root_sig_bindings.h" // D3D12RootSigBindings
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-win32/dx/dx12/tac_dx12_root_sig_builder.h"

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

  static void ShaderInputToRootSigBinding( D3D12_SHADER_INPUT_BIND_DESC& info,
                                           D3D12RootSigBindings* bindings )
  {
    D3D12RootSigBinding::Type type = ShaderInputToRootSigBindType( info.Type );
    D3D12RootSigBinding binding
    {
      .mType = type,
      .mName = info.Name,
      .mCount = ( int )info.BindCount,
    };

    bindings->mBindings.push_back( binding );
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
    case D3D_SIT_CBUFFER:     return D3D12_ROOT_PARAMETER_TYPE_CBV;
    case D3D_SIT_BYTEADDRESS: return D3D12_ROOT_PARAMETER_TYPE_SRV;
    case D3D_SIT_TEXTURE:     return D3D12_ROOT_PARAMETER_TYPE_SRV;
    case D3D_SIT_SAMPLER:     return D3D12_ROOT_PARAMETER_TYPE_SRV;
    default: TAC_ASSERT_INVALID_CASE( Type ); return (D3D12_ROOT_PARAMETER_TYPE)0; 
    }
  }

  static void ShaderInputToRootParam( D3D12_SHADER_INPUT_BIND_DESC& info,
                                                       DX12RootSigBuilder* rootSigBuilder )
  {
    if( info.Type == D3D_SIT_SAMPLER )
      rootSigBuilder->Add...desctorpt table; // cuz :
          //const Array descHeaps = {
          //  ( ID3D12DescriptorHeap* )m_srvHeap,
          //  ( ID3D12DescriptorHeap* )m_samplerHeap,
          //};
          //m_commandList->SetDescriptorHeaps( ( UINT )descHeaps.size(), descHeaps.data() );

    if( info.BindCount == 1 )
    {
      D3D12_ROOT_PARAMETER_TYPE ParameterType = ShaderInputToRootParamType(info.Type);
      TAC_ASSERT( ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV ||
                  ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV ||
                  ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV );

      D3D12_ROOT_DESCRIPTOR1 Descriptor
      {
        .ShaderRegister = info.BindPoint,
        .RegisterSpace = info.Space,
        .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE ,
      };
      rootSigBuilder->AddRootDescriptor(ParameterType, D3D12_SHADER_VISIBILITY_ALL, Descriptor);
    }
    else
    {

      // UINT_MAX represents an unbounded array
      UINT NumDescriptors = info.BindCount == 0 ? UINT_MAX : info.BindCount;

      D3D12_DESCRIPTOR_RANGE_TYPE RangeType = ShaderInputToDescriptorRangeType( info.Type );
      D3D12_DESCRIPTOR_RANGE1 range {
        .RangeType = RangeType,
        .NumDescriptors = NumDescriptors,
        .BaseShaderRegister = info.BindPoint,
        .RegisterSpace = info.Space,
        .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
        .OffsetInDescriptorsFromTableStart = 0,
      };
      rootSigBuilder->AddRootDescriptorTable(D3D12_SHADER_VISIBILITY_ALL, range );
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
    const String preprocessedShader = TAC_CALL( HLSLPreprocess( "assets/hlsl/" + fileName, errors ) );

    const DXCCompileParams input
    {
      .mFileName = fileName,
      .mPreprocessedShader = preprocessedShader,
      .mShaderModel = sShaderModel,
      .mOutputDir = RenderApi::GetShaderOutputPath(),
    };

    DXCCompileOutput output = TAC_CALL( DXCCompile( input, errors ) );


    DX12RootSigBuilder rootSigBuilder( mDevice );

    D3D12RootSigBindings bindings;

    // Here's what im thinking.
    // Every descriptor table has its own root parameter,
    // so there wouldn't be a descriptor table that has CBVs, SRVs, and UAVs.
    // Instead that would be 3 separate root parameters.
    for( D3D12_SHADER_INPUT_BIND_DESC& info : output.mReflInfo.mReflBindings )
    {
      ShaderInputToRootSigBinding( info, &bindings );
      ShaderInputToRootParam( info, &rootSigBuilder );
    }

    PCom< ID3D12RootSignature > rootSignature = TAC_CALL( rootSigBuilder.Build( errors ) );

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

