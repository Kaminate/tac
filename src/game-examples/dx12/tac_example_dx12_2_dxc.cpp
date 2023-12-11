#include "tac_example_dx12_2_dxc.h" // self-inc

#include "src/common/shell/tac_shell.h" // sShellPrefPath
#include "src/common/system/tac_filesystem.h" // Tac::Filesystem
#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h" // TAC_DX12_CALL_RET

// d3d12 must be included before dxcapi
//#include <d3d12.h> // D3D12_SHADER_BYTECODE
//#include <dxcapi.h> // IDxcUtils, IDxcCompiler3, DxcCreateInstance, 
#pragma comment (lib, "dxcompiler.lib" )

namespace Tac::Render
{

  // -----------------------------------------------------------------------------------------------

  // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
  struct DXCArgHelper
  {
    struct Result
    {
      LPCWSTR *pArguments;
      UINT32 argCount;
    };

    void DefineMacro( String s )        { AddArgs( "-D", s ); }
    void SetEntryPoint( String s )      { AddArgs( "-E", s ); }
    void SetTargetProfile( String s )   { AddArgs( "-T", s ); }
    void DisableOptimizations()         { AddArg( "-Od" ); }
    void ColPackMtxs()                  { AddArg( "-Zpc" ); }
    void RowPackMtxs()                  { AddArg( "-Zpr" ); }
    void EnableDebugInfo()              { AddArg( "-Zi" ); }

    void AddArgs( StringView , StringView );
    void AddArg( StringView );

    Result Finalize();

  private:
    Vector< std::wstring > mWStrs;
    Vector< const wchar_t* > mWChars;
  };

  void DXCArgHelper::AddArgs( StringView arg0, StringView arg1 )
  {
    AddArg( arg0 );
    AddArg( arg1 );
  }

  void DXCArgHelper::AddArg( StringView arg )
  {
    std::wstring ws;
    for( char c : arg )
      ws += c;
    mWStrs.push_back( ws );
  }

  DXCArgHelper::Result DXCArgHelper::Finalize()
    {
      const int n = mWStrs.size();
      mWChars.resize( n );
      for( int i =0; i < n; ++i )
        mWChars[i] = mWStrs[i].c_str();

      return
      {
        mWChars.data(),
        (UINT32)mWChars.size(),
      };
    }

  // -----------------------------------------------------------------------------------------------

  static String GetTarget( ShaderType type, D3D_SHADER_MODEL model )
  {
    const String shaders[] =
    {
      "vs",
      "ps",
      "cs",
    };
    static_assert( TAC_ARRAY_SIZE( shaders ) == ( int )ShaderType::Count );
    TAC_ASSERT_INDEX( type, ShaderType::Count );
    return shaders[ ( int )type ]
      + "_" + ( '0' + ( char )( ( int )model / 16 ) )
      + "_" + ( '0' + ( char )( ( int )model % 16 ) );
  }

  // -----------------------------------------------------------------------------------------------

  // ext does not include '.'
  static void SaveBlobToFile( const void* bytes,
                              const int byteCount,
                              const StringView stem,
                              const StringView ext,
                              Errors& errors )
  {
    const String filename = String() + stem + '.' + ext; 
    const std::filesystem::path path = sShellPrefPath.Get() / filename.c_str();
    TAC_CALL( Filesystem::SaveToFile, path, bytes, byteCount, errors );
  }
  static void SaveBlobToFile( TAC_NOT_CONST PCom< IDxcBlob> blob,
                              const StringView stem,
                              const StringView ext,
                              Errors& errors )
  {
    const void* bytes = blob->GetBufferPointer();
    const int byteCount = ( int )blob->GetBufferSize();
    TAC_CALL( SaveBlobToFile,bytes,byteCount, stem, ext, errors );
  }

  // -----------------------------------------------------------------------------------------------

  DX12DXCOutput DX12CompileShaderDXC( const DX12ShaderCompileFromStringInput& input,
                                             Errors& errors )
  {

    TAC_ASSERT_MSG(
      input.mShaderModel >= D3D_SHADER_MODEL_6_0,
      "Specifically using dxc instead of d3dcompiler to support a newer shader model" );

    // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
    PCom<IDxcUtils> pUtils;
    PCom<IDxcCompiler3> pCompiler;
    TAC_DX12_CALL_RET( {}, DxcCreateInstance, CLSID_DxcUtils, pUtils.iid(), pUtils.ppv() );
    TAC_DX12_CALL_RET( {}, DxcCreateInstance, CLSID_DxcCompiler, pCompiler.iid(), pCompiler.ppv() );

    const String target = GetTarget( input.mType, input.mShaderModel );
    const String pShaderStem = "foo";

    const std::filesystem::path pShaderName{ ( pShaderStem + ".hlsl" ).data() };

    const void* shaderBytes = input.mPreprocessedShader.data();
    const int shaderByteCount = input.mPreprocessedShader.size();
    TAC_CALL_RET( {}, SaveBlobToFile, shaderBytes, shaderByteCount, pShaderStem, "hlsl", errors );

    TAC_NOT_CONST DXCArgHelper argHelper;
    argHelper.SetEntryPoint( input.mEntryPoint );
    argHelper.SetTargetProfile( target );
    if( IsDebugMode )
    {
      argHelper.EnableDebugInfo();
      argHelper.DisableOptimizations();
    }

    const auto [ pArguments, argCount ] = argHelper.Finalize(); 

    const DxcBuffer Source
    {
      .Ptr = input.mPreprocessedShader.data(),
      .Size = (SIZE_T)input.mPreprocessedShader.size(),
      .Encoding = DXC_CP_ACP,
    };

    PCom<IDxcResult> pResults;
    HRESULT compileHR = pCompiler->Compile( &Source,
      pArguments,
      argCount,
      nullptr,
      pResults.iid(),
      pResults.ppv() );

    TAC_ASSERT( SUCCEEDED( compileHR  ) );

    const auto outN = pResults->GetNumOutputs();
    Vector< DXC_OUT_KIND > outKinds( (int)outN );
    for( UINT32 i = 0; i < outN; ++i )
      outKinds[ i ] = pResults->GetOutputByIndex(i);


    //
    // Print errors if present.
    //
    PCom<IDxcBlobUtf8> pErrors;
    pResults->GetOutput( DXC_OUT_ERRORS, pErrors.iid(), pErrors.ppv(), nullptr );
    const StringView errorSV = pErrors
      ? StringView( pErrors->GetStringPointer(), (int)pErrors->GetStringLength() )
      : StringView();

    // Note that d3dcompiler would return null if no errors or warnings are present.
    // IDxcCompiler3::Compile will always return an error buffer, but its length
    // will be zero if there are no warnings or errors.
    if( !errorSV.empty() )
    {
      TAC_ASSERT_CRITICAL( errorSV );
    }

    //
    // Quit if the compilation failed.
    //
    HRESULT hrStatus;
    pResults->GetStatus( &hrStatus );
    if( FAILED( hrStatus ) )
    {
      TAC_ASSERT_CRITICAL( "Compilation failed" );
    }

    //
    // Save shader binary.
    //
    PCom<IDxcBlob> pShader;
    TAC_DX12_CALL_RET( {},
                       pResults->GetOutput,
                       DXC_OUT_OBJECT,
                       pShader.iid(),
                       pShader.ppv(),
                       nullptr );
    TAC_ASSERT( pShader );
    TAC_CALL_RET( {}, SaveBlobToFile, pShader, pShaderStem, "bin", errors );

    //
    // Save pdb.
    //

    PCom<IDxcBlob> pPDB ;
    PCom<IDxcBlobUtf16> pPDBName ;
    pResults->GetOutput( DXC_OUT_PDB,
                         pPDB.iid(),
                         pPDB.ppv(),
                         pPDBName.CreateAddress() );
    TAC_ASSERT( pPDB );
    TAC_CALL_RET( {}, SaveBlobToFile, pPDB, pShaderStem, "pdb", errors );

    const DX12DXCOutput output
    {
      .mBlob = pShader,
      .mByteCode = 
      {
        .pShaderBytecode = pShader->GetBufferPointer(),
        .BytecodeLength = pShader->GetBufferSize(),
      },
    };

    return output;
  }
} // namespace Tac

