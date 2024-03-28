#include "tac_dxc.h" // self-inc
#include "tac_dxc_arg_helper.h"

#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/string/tac_string_util.h" // IsAscii
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL_RET

#include <d3d12shader.h> // Shader reflection (ID3D12ShaderReflection)

// d3d12 must be included before dxcapi
//#include <d3d12.h> // D3D12_SHADER_BYTECODE
//#include <dxcapi.h> // IDxcUtils, IDxcCompiler3, DxcCreateInstance, 
#pragma comment (lib, "dxcompiler.lib" )

namespace Tac::Render
{
  struct ShaderTypeData
  {
    String             GetTarget( D3D_SHADER_MODEL model ) const
    {
      // example return value: "vs_6_1"
      return String() + String( 1, Tac::ToLower( mLetter ) ) + "s"
        + "_" + ( '0' + ( char )( ( int )model / 16 ) )
        + "_" + ( '0' + ( char )( ( int )model % 16 ) );
    }

    Optional< String > FindEntryPoint( StringView shader ) const
    {
      String entryPoint0 = String( 1, Tac::ToUpper( mLetter ) ) + "S";
      String entryPoint1 = entryPoint0 + "Main";
      StringView entryPoints[] = { entryPoint0, entryPoint1 };

      for( StringView& entryPoint : entryPoints )
      {
        String search = entryPoint + "(";
        if( shader.contains( search ) )
        {
          return ( String )entryPoint;
        }
      }

      return {};
    }

    char mLetter;
  };

  static const ShaderTypeData sVSData{ .mLetter = 'v' };
  static const ShaderTypeData sPSData{ .mLetter = 'p' };

  // -----------------------------------------------------------------------------------------------

  static void SaveBlobToFile( TAC_NOT_CONST PCom< IDxcBlob> blob,
                              const Filesystem::Path& path,
                              Errors& errors )
  {
    const void* bytes = blob->GetBufferPointer();
    const int byteCount = ( int )blob->GetBufferSize();
    TAC_CALL( Filesystem::SaveToFile( path, bytes, byteCount, errors ) );
  }

  static String GetBlob16AsUTF8( PCom< IDxcBlobUtf16> blob16, PCom<IDxcUtils> pUtils )
  {
    if( !blob16 )
      return {};

    PCom< IDxcBlobUtf8> pName8;
    TAC_ASSERT( SUCCEEDED( pUtils->GetBlobAsUtf8(
      ( IDxcBlob* )blob16,
      pName8.CreateAddress() ) ) );
    return String( pName8->GetStringPointer(), ( int )pName8->GetStringLength() );
  }

  static bool DidCompileSucceed( PCom<IDxcResult> pResults )
  {
    HRESULT hrStatus;
    TAC_ASSERT( SUCCEEDED( pResults->GetStatus( &hrStatus ) ) );
    return SUCCEEDED( hrStatus );
  }


  static void PrintCompilerInfo( IDxcPdbUtils* pdbUtils, IDxcBlob* pPDB)
  {
    static bool printed;
    if( printed )
      return;

    printed = true;

    if constexpr ( not IsDebugMode )
      return;


    if( S_OK != pdbUtils->Load( pPDB ) )
      return;

    PCom<IDxcVersionInfo> verInfo;

    if( S_OK != pdbUtils->GetVersionInfo( verInfo.CreateAddress() ) )
      return;


    UINT32 commitCount{};
    char* commitHash{};
    UINT32 flags{};
    UINT32 major{};
    UINT32 minor{};
    if( PCom<IDxcVersionInfo2> verInfo2 = verInfo.QueryInterface<IDxcVersionInfo2>() )
    {
      verInfo2->GetCommitInfo( &commitCount, &commitHash );
      verInfo2->GetFlags( &flags );
      verInfo2->GetVersion( &major, &minor );
    }

    char* ver = nullptr;
    if( PCom<IDxcVersionInfo3> verInfo3 = verInfo.QueryInterface<IDxcVersionInfo3>() )
    {
      verInfo3->GetCustomVersionString( &ver );
    }

    Vector< String > strs;
    strs.push_back( String() + "Dxc Compiler commit count "
                    + Tac::ToString( commitCount ) + " hash " + commitHash );
    strs.push_back( String() + "Flags: " + Tac::ToString( flags ) );
    strs.push_back( String() + "Version: "
                    + Tac::ToString( major ) + "."
                    + Tac::ToString( minor ) );
    strs.push_back( String() + "Custom Version: " + ( ver ? ver : "n/a" ) );

    OS::OSDebugPrintLine( AsciiBoxAround( Join( strs, "\n" ) ) );
  }


  static PCom< IDxcBlob > DXCCompileBlob( const ShaderTypeData& typeData,
                                          const DXCCompileParams& input,
                                          Errors& errors  )
  {
    TAC_ASSERT( !input.mOutputDir.empty() );

    Optional<String> entryPoint = typeData.FindEntryPoint( input.mPreprocessedShader );
    if( !entryPoint.HasValue() )
      return {};

    TAC_ASSERT_MSG(
      input.mShaderModel >= D3D_SHADER_MODEL_6_0,
      "Specifically using dxc instead of d3dcompiler to support a newer shader model" );

    // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

    PCom< IDxcCompiler3 > pCompiler;
    TAC_DX12_CALL_RET( {}, DxcCreateInstance( CLSID_DxcCompiler, pCompiler.iid(), pCompiler.ppv() ) );

    PCom< IDxcUtils > pUtils;
    TAC_DX12_CALL_RET( {}, DxcCreateInstance( CLSID_DxcUtils, pUtils.iid(), pUtils.ppv() ) );

    PCom< IDxcPdbUtils > pdbUtils;
    TAC_DX12_CALL_RET( {}, DxcCreateInstance( CLSID_DxcPdbUtils, pdbUtils.iid(), pdbUtils.ppv() ) );



    // this shit don't work (try and get the version)
    if( false )
    {
      //PCom<IDxcCompilerArgs> mArgs;
      TAC_NOT_CONST Array args = { L"--version" };
      //const HRESULT hr = mArgs->AddArgumentsUTF8( args.data(), args.size() );
      PCom<IDxcResult> pResults;

      // E_INVALIDARG
      HRESULT hr = pCompiler->Compile( nullptr,
                          args.data(),
                          args.size(),
                          nullptr,
                          pResults.iid(),
                          pResults.ppv() );

      const UINT32 n = pResults->GetNumOutputs();
      for( UINT32 i = 0; i < n; ++i )
      {
        DXC_OUT_KIND kind = pResults->GetOutputByIndex( i );
        ++asdf;
      }
      ++asdf;
    }

    const String target = typeData.GetTarget( input.mShaderModel );
    const String inputShaderName =  input.mFileName;
    const Filesystem::Path hlslShaderPath = input.mOutputDir / inputShaderName;

    TAC_CALL_RET( {}, Filesystem::SaveToFile( hlslShaderPath, input.mPreprocessedShader, errors ) );


    TAC_NOT_CONST DXCArgHelper::Params argHelperSetup
    {
      .mEntryPoint = entryPoint.GetValueUnchecked(),
      .mTargetProfile = target,
      .mFilename = inputShaderName,
      .mPDBDir = input.mOutputDir,
      .mUtils = pUtils,
    };
    TAC_NOT_CONST DXCArgHelper argHelper( argHelperSetup );

    const auto pArguments = argHelper.GetArgs();
    const auto argCount = argHelper.GetArgCount();

    const DxcBuffer Source
    {
      .Ptr = input.mPreprocessedShader.data(),
      .Size = (SIZE_T)input.mPreprocessedShader.size(),
      .Encoding = DXC_CP_ACP,
    };

    PCom<IDxcResult> pResults;
    TAC_ASSERT( SUCCEEDED( pCompiler->Compile(
      &Source,
      pArguments,
      argCount,
      nullptr,
      pResults.iid(),
      pResults.ppv() ) ) );

    //
    // Quit if the compilation failed.
    //
    if( !DidCompileSucceed( pResults ) )
    {
      PCom<IDxcBlobUtf8> pErrors;
      String errorStr = "Shader compilation failed";
      if( pResults->HasOutput( DXC_OUT_ERRORS ) )
      {

        //
        // Print errors if present.
        //
        TAC_ASSERT( SUCCEEDED( pResults->GetOutput(
          DXC_OUT_ERRORS,
          pErrors.iid(),
          pErrors.ppv(),
          nullptr ) ) );

        errorStr += '\n';
        errorStr += StringView( pErrors->GetStringPointer(),
                                ( int )pErrors->GetStringLength() );
      }



      TAC_RAISE_ERROR_RETURN( errorStr, {} );
    }

    PCom<IDxcBlob> reflBlob;
    if( pResults->HasOutput( DXC_OUT_REFLECTION ) )
    {
      TAC_ASSERT( SUCCEEDED( pResults->GetOutput(
        DXC_OUT_REFLECTION,
        reflBlob.iid(),
        reflBlob.ppv(),
        nullptr ) ) );

      const DxcBuffer reflBuf
      {
          .Ptr = reflBlob->GetBufferPointer(),
          .Size = reflBlob->GetBufferSize(),
          .Encoding = 0,
      };

      PCom< ID3D12ShaderReflection > shaderReflection{};
      TAC_ASSERT( SUCCEEDED( pUtils->CreateReflection(
        &reflBuf,
        shaderReflection.iid(),
        shaderReflection.ppv() ) ) );

      D3D12_SHADER_DESC shaderDesc{};
      shaderReflection->GetDesc( &shaderDesc );

      for( UINT iCBuf = 0; iCBuf < shaderDesc.ConstantBuffers; ++iCBuf )
      {
        ID3D12ShaderReflectionConstantBuffer* cBuf =
          shaderReflection->GetConstantBufferByIndex( iCBuf );

        D3D12_SHADER_BUFFER_DESC desc{}; // ie CBufferPerFrame, CBufferPerObject
        cBuf->GetDesc( &desc );

        ++asdf;
      }

      for( UINT iRsc = 0; iRsc < shaderDesc.BoundResources; ++iRsc )
      {
        // ie: CBufferPerFrame, CBufferPerObject, "linearSampler", "image"
        D3D12_SHADER_INPUT_BIND_DESC desc; 
        shaderReflection->GetResourceBindingDesc( iRsc, &desc );
        ++asdf;
      }

      for( UINT iInput = 0; iInput < shaderDesc.InputParameters; ++iInput )
      {
        // ie: POSITION, TEXCOORD, etc
        D3D12_SIGNATURE_PARAMETER_DESC inputParamDesc;
        shaderReflection->GetInputParameterDesc( iInput, &inputParamDesc );
        ++asdf;
      }

      for( UINT iOutput = 0; iOutput < shaderDesc.OutputParameters; ++iOutput )
      {
        // ie: SV_POSITION, SV_TARGET, etc
        D3D12_SIGNATURE_PARAMETER_DESC desc;
        shaderReflection->GetOutputParameterDesc( iOutput, &desc );
        ++asdf;
      }


      shaderDesc.Creator; // <-- note this shows dxc ver info

    }
    

    PCom<IDxcBlob> pShader;
    if( pResults->HasOutput( DXC_OUT_OBJECT ) )
    {
      //
      // Save shader binary.
      //
      PCom< IDxcBlobUtf16 > pShaderName;
      TAC_DX12_CALL_RET( {},
                         pResults->GetOutput(
                         DXC_OUT_OBJECT,
                         pShader.iid(),
                         pShader.ppv(),
                         pShaderName.CreateAddress() ) );
      TAC_RAISE_ERROR_IF_RETURN( !pShader, "No shader dxil", {} );
      const String outputShaderName = GetBlob16AsUTF8( pShaderName, pUtils );
      const Filesystem::Path dxilShaderPath = input.mOutputDir / outputShaderName;
      TAC_CALL_RET( {}, SaveBlobToFile(pShader, dxilShaderPath, errors ));
    }
    else
    {
      TAC_RAISE_ERROR_RETURN( "no object", {} );
    }

    if( pResults->HasOutput( DXC_OUT_PDB ) )
    {
      //
      // Save pdb.
      //

      PCom<IDxcBlob> pPDB ;
      PCom<IDxcBlobUtf16> pPDBName ;
      TAC_DX12_CALL_RET( {},
                         pResults->GetOutput( DXC_OUT_PDB,
                         pPDB.iid(),
                         pPDB.ppv(),
                         pPDBName.CreateAddress() ) );
      TAC_RAISE_ERROR_IF_RETURN( !pShader, "No shader pdb", {} );
      const String pdbName = GetBlob16AsUTF8( pPDBName, pUtils );
      const Filesystem::Path pdbPath = input.mOutputDir / pdbName;
      TAC_CALL_RET( {}, SaveBlobToFile(pPDB, pdbPath, errors ));

      PrintCompilerInfo( pdbUtils.Get(), pPDB.Get() );

    }

    return pShader;
  }

} // namespace Tac::Render

namespace Tac
{
  Render::DXCCompileOutput Render::DXCCompile( const DXCCompileParams& input, Errors& errors )
  {
    PCom< IDxcBlob > vsBlob;
    PCom< IDxcBlob > psBlob;

    vsBlob = DXCCompileBlob( sVSData, input, errors );
    psBlob = DXCCompileBlob( sPSData, input, errors );

    TAC_RAISE_ERROR_IF_RETURN( !vsBlob && !psBlob, "Failed to find any shaders", {} );

    return DXCCompileOutput
    {
      .mVSBlob = vsBlob,
      .mPSBlob = psBlob,
    };
  }

}
