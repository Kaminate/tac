#include "tac_dxc.h" // self-inc
#include "tac_dxc_arg_helper.h"

#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/string/tac_string_util.h" // IsAscii
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL_RET

#include <d3d12shader.h> // Shader reflection (ID3D12ShaderReflection)

// d3d12 must be included before dxcapi
//#include <d3d12.h> // D3D12_SHADER_BYTECODE
//#include <dxcapi.h> // IDxcUtils, IDxcCompiler3, DxcCreateInstance, 
#pragma comment (lib, "dxcompiler.lib" )

namespace Tac::Render
{
  struct ShaderCompiler
  {

    struct DXCCompileBlobInputs
    {
       dynmc DXCReflInfo*      mReflInfo;
       const DXCCompileParams& mCompileParams;
    };

    String             GetTarget( D3D_SHADER_MODEL model ) const
    {
      // example return value: "vs_6_1"
      return String() + String( 1, Tac::ToLower( mLetter ) ) + "s"
        + "_" + ( '0' + ( char )( ( int )model / 16 ) )
        + "_" + ( '0' + ( char )( ( int )model % 16 ) );
    }

    Optional< String > FindEntryPoint( StringView shader ) const
    {
      String entryPoint0 { String( 1, Tac::ToUpper( mLetter ) ) + "S" };
      String entryPoint1 { entryPoint0 + "Main" };
      StringView entryPoints[]  { entryPoint0, entryPoint1 };

      for( StringView& entryPoint : entryPoints )
      {
        String search { entryPoint + "(" };
        if( shader.contains( search ) )
        {
          return ( String )entryPoint;
        }
      }

      return {};
    }

    PCom< IDxcBlob >   DXCCompileBlob( DXCCompileBlobInputs, Errors& ) const;

    char mLetter;
    bool mReflectShaderInputs;
  };

  static const ShaderCompiler sVSData{ .mLetter { 'v' }, .mReflectShaderInputs{ true }, };
  static const ShaderCompiler sPSData{ .mLetter { 'p' }, .mReflectShaderInputs{ false }, };
  static const ShaderCompiler sCSData{ .mLetter { 'c' }, .mReflectShaderInputs{ true }, };
  static const bool           sVerbose;
  static dynmc bool           sPrintedCompilerInfo;

  // -----------------------------------------------------------------------------------------------

  static void SaveBlobToFile( dynmc PCom< IDxcBlob> blob,
                              const FileSys::Path& path,
                              Errors& errors )
  {
    const void* bytes { blob->GetBufferPointer() };
    const int byteCount { ( int )blob->GetBufferSize() };
    TAC_CALL( FileSys::SaveToFile( path, bytes, byteCount, errors ) );
  }

  static String GetBlob16AsUTF8( IDxcBlobUtf16* blob16, IDxcUtils* pUtils )
  {
    if( !blob16 )
      return {};

    PCom< IDxcBlobUtf8> pName8;
    TAC_ASSERT( SUCCEEDED( pUtils->GetBlobAsUtf8(
      ( IDxcBlob* )blob16,
      pName8.CreateAddress() ) ) );
    return String( pName8->GetStringPointer(), ( int )pName8->GetStringLength() );
  }


  static void PrintCompilerInfo( IDxcPdbUtils* pdbUtils, IDxcBlob* pPDB )
  {
    if( sPrintedCompilerInfo )
      return;


    if constexpr( not kIsDebugMode )
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
    if( PCom<IDxcVersionInfo2> verInfo2{ verInfo.QueryInterface<IDxcVersionInfo2>() } )
    {
      verInfo2->GetCommitInfo( &commitCount, &commitHash );
      verInfo2->GetFlags( &flags );
      verInfo2->GetVersion( &major, &minor );
    }

    char* ver {};
    if( PCom<IDxcVersionInfo3> verInfo3{ verInfo.QueryInterface<IDxcVersionInfo3>() } )
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
    sPrintedCompilerInfo = true;
  }

  bool DXCReflInfo::HasBinding( StringView name )
  {
    for( const D3D12_SHADER_INPUT_BIND_DESC& existingBinding : mReflBindings )
      if( StringView( existingBinding.Name ) == name )
        return true;
    return false;
  }

  void DXCReflInfo::AddBinding( D3D12_SHADER_INPUT_BIND_DESC desc )
  {
    if( HasBinding( desc.Name ) )
      return;

    mReflBindings.push_back( desc );
  }

  static D3D12_SHADER_BYTECODE GetBytecode( const PCom< IDxcBlob >& blob )
  {
    dynmc IDxcBlob* pBlob{ blob.Get() };
    return pBlob ?
      D3D12_SHADER_BYTECODE{ pBlob->GetBufferPointer(), pBlob->GetBufferSize() } :
      D3D12_SHADER_BYTECODE{};
  }

  static void ReflectShader( IDxcUtils* pUtils,
                             IDxcResult* pResults,
                             DXCReflInfo* reflInfo,
                             bool reflectShaderInputs )
  {
    PCom< IDxcBlob > reflBlob;
    if( !pResults->HasOutput( DXC_OUT_REFLECTION ) )
      return;

    TAC_ASSERT( SUCCEEDED( pResults->GetOutput(
      DXC_OUT_REFLECTION,
      reflBlob.iid(),
      reflBlob.ppv(),
      nullptr ) ) );

    const DxcBuffer reflBuf
    {
        .Ptr { reflBlob->GetBufferPointer() },
        .Size { reflBlob->GetBufferSize() },
        .Encoding {},
    };

    PCom< ID3D12ShaderReflection > shaderReflection{};
    TAC_ASSERT( SUCCEEDED( pUtils->CreateReflection(
      &reflBuf,
      shaderReflection.iid(),
      shaderReflection.ppv() ) ) );

    D3D12_SHADER_DESC shaderDesc{};
    shaderReflection->GetDesc( &shaderDesc );

    for( UINT iCBuf {}; iCBuf < shaderDesc.ConstantBuffers; ++iCBuf )
    {
      ID3D12ShaderReflectionConstantBuffer* cBuf { shaderReflection->GetConstantBufferByIndex( iCBuf ) };

      D3D12_SHADER_BUFFER_DESC desc{}; // ie CBufferPerFrame, CBufferPerObject
      cBuf->GetDesc( &desc );

      ++asdf;
    }

    for( UINT iRsc {}; iRsc < shaderDesc.BoundResources; ++iRsc )
    {
      // ie: CBufferPerFrame, CBufferPerObject, "linearSampler", "image"
      D3D12_SHADER_INPUT_BIND_DESC desc; 
      shaderReflection->GetResourceBindingDesc( iRsc, &desc );
      ++asdf;

      // see also definition of D3D12_SHADER_INPUT_BIND_DESC for desc of each parameter

      if( desc.Type == D3D_SIT_BYTEADDRESS ) // or D3D_SIT_TEXTURE
      {
      }
      if( desc.Dimension == D3D_SRV_DIMENSION_BUFFER ) // or D3D_SRV_DIMENSION_TEXTURE2D
      {
        desc.BindCount ; // size of buffer, if fixed, 0 if unbounded
      }

      reflInfo->AddBinding( desc );
    }

    for( UINT iInput {}; iInput < shaderDesc.InputParameters; ++iInput )
    {
      // ie: POSITION, TEXCOORD, etc
      D3D12_SIGNATURE_PARAMETER_DESC inputParamDesc;
      shaderReflection->GetInputParameterDesc( iInput, &inputParamDesc );

      if( inputParamDesc.SystemValueType != D3D_NAME_UNDEFINED )
        continue; // don't reflect SV_ semantics

      if( reflectShaderInputs )
      {
        const DXCReflInfo::Input input
        {
          .mName     { inputParamDesc.SemanticName },
          .mIndex    { ( int )inputParamDesc.SemanticIndex },
          .mRegister { ( int )inputParamDesc.Register },
        };
        reflInfo->mInputs.push_back( input );
      }
      ++asdf;
    }

    for( UINT iOutput {}; iOutput < shaderDesc.OutputParameters; ++iOutput )
    {
      // ie: SV_POSITION, SV_TARGET, etc
      D3D12_SIGNATURE_PARAMETER_DESC desc;
      shaderReflection->GetOutputParameterDesc( iOutput, &desc );
      ++asdf;
    }

    reflInfo->mReflBlobs.push_back( reflBlob );
    reflInfo->mRefls.push_back( shaderReflection );

    shaderDesc.Creator; // <-- note this shows dxc ver info
  }

  static void SavePDB( IDxcUtils* pUtils,
                       IDxcPdbUtils* pdbUtils,
                       IDxcResult* pResults,
                       IDxcBlob* pShader,
                       FileSys::Path outputDir,
                       Errors& errors )
  {
    if( !pResults->HasOutput( DXC_OUT_PDB ) )
      return;
    PCom<IDxcBlob> pPDB ;
    PCom<IDxcBlobUtf16> pPDBName ;
    TAC_DX12_CALL( pResults->GetOutput( DXC_OUT_PDB,
                       pPDB.iid(),
                       pPDB.ppv(),
                       pPDBName.CreateAddress() ) );
    TAC_RAISE_ERROR_IF( !pShader, "No shader pdb" );
    const String pdbName { GetBlob16AsUTF8( pPDBName.Get(), pUtils ) };
    const FileSys::Path pdbPath { outputDir / pdbName };
    TAC_CALL( SaveBlobToFile( pPDB, pdbPath, errors ) );

    if( sVerbose )
      PrintCompilerInfo( pdbUtils, pPDB.Get() );
  }

  static void CheckCompileSuccess( IDxcResult* pResults, Errors& errors )
  {
    HRESULT compileStatus;
    TAC_RAISE_ERROR_IF( FAILED( pResults->GetStatus( &compileStatus ) ),
                        "Failed to get shader compilation status" );
    if( SUCCEEDED( compileStatus ) )
      return;

    TAC_RAISE_ERROR_IF( !pResults->HasOutput( DXC_OUT_ERRORS ),
                        "Shader compilation failed, no error blob" );

    PCom<IDxcBlobUtf8> pErrors;
    TAC_RAISE_ERROR_IF( FAILED( pResults->GetOutput(
      DXC_OUT_ERRORS,
      pErrors.iid(),
      pErrors.ppv(),
      nullptr ) ),
      "Shader compilation failed and error blob retrieval failed" );

    StringView errorBlobStr( pErrors->GetStringPointer(),
                             ( int )pErrors->GetStringLength() );

    TAC_RAISE_ERROR( String() + "Shader compilation failed: " + errorBlobStr );
  }


  PCom< IDxcBlob > ShaderCompiler::DXCCompileBlob( DXCCompileBlobInputs inputs,
                                                   Errors& errors ) const
  {
    const Optional< String > optEntryPoint{
      FindEntryPoint( inputs.mCompileParams.mPreprocessedShader ) };
    if( !optEntryPoint.HasValue() )
      return {};

    const String entryPoint{ optEntryPoint.GetValue() };

    const String target{ GetTarget( inputs.mCompileParams.mShaderModel ) };

    DXCReflInfo* reflInfo{ inputs.mReflInfo };
    const DXCCompileParams& input{ inputs.mCompileParams };
    //bool                    reflectShaderInputs{ inputs.reflectShaderInputs };

    TAC_ASSERT( !input.mOutputDir.empty() );
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
      dynmc Array args  { L"--version" };
      //const HRESULT hr = mArgs->AddArgumentsUTF8( args.data(), args.size() );
      PCom<IDxcResult> pResults;

      // E_INVALIDARG
      HRESULT hr{ pCompiler->Compile( nullptr,
                          args.data(),
                          args.size(),
                          nullptr,
                          pResults.iid(),
                          pResults.ppv() ) };

      const UINT32 n { pResults->GetNumOutputs() };
      for( UINT32 i {}; i < n; ++i )
      {
        DXC_OUT_KIND kind { pResults->GetOutputByIndex( i ) };
        ++asdf;
      }
      ++asdf;
    }

    dynmc DXCArgHelper::Params argHelperSetup
    {
      .mEntryPoint    { entryPoint },
      .mTargetProfile { target },
      .mFilename      { inputs.mCompileParams.mFileName },
      .mPDBDir        { input.mOutputDir },
      .mUtils         { pUtils },
    };
    dynmc DXCArgHelper argHelper( argHelperSetup );

    const DxcBuffer Source
    {
      .Ptr      { input.mPreprocessedShader.data() },
      .Size     { ( SIZE_T )input.mPreprocessedShader.size() },
      .Encoding { DXC_CP_ACP },
    };

    PCom< IDxcResult > pResults;

    const HRESULT compileHR{
      pCompiler->Compile( &Source,
                          argHelper.GetArgs() ,
                          argHelper.GetArgCount() ,
                          nullptr,
                          pResults.iid(),
                          pResults.ppv() ) };
    TAC_ASSERT( SUCCEEDED( compileHR ) );

    CheckCompileSuccess( pResults.Get(), errors );

    ReflectShader( pUtils.Get(), pResults.Get(), reflInfo, mReflectShaderInputs );
    
    TAC_RAISE_ERROR_IF_RETURN( {}, !pResults->HasOutput( DXC_OUT_OBJECT ), "no shader binary" );
    PCom< IDxcBlob > pShader;
    PCom< IDxcBlobUtf16 > pShaderName;
    TAC_DX12_CALL_RET( {},
                       pResults->GetOutput(
                       DXC_OUT_OBJECT,
                       pShader.iid(),
                       pShader.ppv(),
                       pShaderName.CreateAddress() ) );
    TAC_RAISE_ERROR_IF_RETURN( {}, !pShader, "No shader dxil" );
    const String outputShaderName { GetBlob16AsUTF8( pShaderName.Get(), pUtils.Get() ) };
    const FileSys::Path dxilShaderPath { input.mOutputDir / outputShaderName };
    TAC_CALL_RET( SaveBlobToFile( pShader, dxilShaderPath, errors ) );

    SavePDB( pUtils.Get(),
             pdbUtils.Get(),
             pResults.Get(),
             pShader.Get(),
             input.mOutputDir,
             errors );

    return pShader;
  }


  D3D12_SHADER_BYTECODE DXCCompileOutput::GetVSBytecode() const { return GetBytecode( mVSBlob ); }
  D3D12_SHADER_BYTECODE DXCCompileOutput::GetPSBytecode() const { return GetBytecode( mPSBlob ); };
  D3D12_SHADER_BYTECODE DXCCompileOutput::GetCSBytecode() const { return GetBytecode( mCSBlob ); };
} // namespace Tac::Render

namespace Tac
{
  Render::DXCCompileOutput Render::DXCCompile( const DXCCompileParams& input, Errors& errors )
  {
    DXCReflInfo reflInfo;

    const ShaderCompiler::DXCCompileBlobInputs blobInput
    {
      .mReflInfo            { &reflInfo },
      .mCompileParams       { input },
    };

    const StringView fileName{ input.mFileName };

    {
#if 0
      const int iSlash{ input.mFileName.find_last_of( "/\\" ) };
      const StringView inputShaderName{
        iSlash == StringView::npos ? input.mFileName : input.mFileName.substr( iSlash + 1 ) };
      const FileSys::Path hlslShaderPath { input.mOutputDir / inputShaderName };
      TAC_CALL_RET( FileSys::SaveToFile( hlslShaderPath, input.mPreprocessedShader, errors ) );
#else
      // ???? why
      TAC_ASSERT( !fileName.contains( "/\\" ) );
      const FileSys::Path hlslShaderPath { input.mOutputDir / fileName };
      TAC_CALL_RET( FileSys::SaveToFile( hlslShaderPath, input.mPreprocessedShader, errors ) );
#endif
    }

    TAC_CALL_RET( PCom< IDxcBlob > vsBlob { sVSData.DXCCompileBlob( blobInput, errors ) } );
    TAC_CALL_RET( PCom< IDxcBlob > psBlob { sPSData.DXCCompileBlob( blobInput, errors ) } );
    TAC_CALL_RET( PCom< IDxcBlob > csBlob { sCSData.DXCCompileBlob( blobInput, errors ) } );

    TAC_RAISE_ERROR_IF_RETURN( {}, !( vsBlob || psBlob || csBlob ),
                               String() + "Shader " + fileName + " compiled no blobs" );

    return DXCCompileOutput
    {
      .mVSBlob   { vsBlob },
      .mPSBlob   { psBlob },
      .mCSBlob   { csBlob },
      .mReflInfo { reflInfo },
    };
  }


} // namespace Tac
