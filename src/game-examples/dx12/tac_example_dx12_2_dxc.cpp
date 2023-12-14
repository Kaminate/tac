#include "tac_example_dx12_2_dxc.h" // self-inc

#include "src/common/shell/tac_shell.h" // sShellPrefPath
#include "src/common/system/tac_filesystem.h" // Tac::Filesystem
#include "src/common/containers/tac_array.h"
#include "src/common/string/tac_string_util.h" // IsAscii
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
    struct BasicSetup
    {
      String mEntryPoint;
      String mTargetProfile;
      String mFilename;
      Filesystem::Path mPDBDir;
      PCom<IDxcUtils> mUtils;
    };

    DXCArgHelper( TAC_NOT_CONST BasicSetup& );

    void SetEntryPoint( String s )         { AddArgs( "-E", s ); }
    void SetTargetProfile( String s )      { AddArgs( "-T", s ); }
    void SetFilename( String );
    void DefineMacro( String s )           { AddArgs( "-D", s ); }
    void ColPackMtxs()                     { AddArg( "-Zpc" ); }
    void RowPackMtxs()                     { AddArg( "-Zpr" ); }
    void DisableOptimizations()            { AddArg( "-Od" ); }
    void EnableDebug()                     { AddArg( "-Zi" ); } // enable pdb
    void StripBytecodeDebug()              { AddArg( "-Qstrip_debug" ); }
    void StripBytecodeReflection()         { AddArg( "-Qstrip_reflect" ); }
    void StripBytecodeRootSignature()      { AddArg( "-Qstrip_rootsignature" ); }
    //void StripBytecodePrivateData()        { AddArg( "-Qstrip_priv" ); }
    void SaveReflection();
    void SaveBytecode();
    void SaveDebug( const Filesystem::Path& pdbDir );
    void AddArgs( StringView , StringView );
    void AddArg( StringView );

    auto GetArgs()     { return mArgs->GetArguments(); }
    auto GetArgCount() { return mArgs->GetCount(); }


  private:
    std::wstring ToWStr(StringView);
    PCom<IDxcCompilerArgs>   mArgs;
    String                   mFilename;
    String                   mStem;
  };

  std::wstring DXCArgHelper::ToWStr(StringView s)
  {
    std::wstring result;
    for( char c : s )
      result += c;
    return result;
  }

  DXCArgHelper::DXCArgHelper( TAC_NOT_CONST BasicSetup& setup )
  {
    TAC_ASSERT(!setup.mFilename.empty());

    const HRESULT hr = setup.mUtils->BuildArguments(
      ToWStr( setup.mFilename ).data(),
      ToWStr( setup.mEntryPoint ).data(),
      ToWStr( setup.mTargetProfile ).data(),
      nullptr,
      0,
      nullptr,
      0,
      mArgs.CreateAddress() );
    TAC_ASSERT( SUCCEEDED(hr) );


    //SetTargetProfile(setup.mTargetProfile);
    //SetEntryPoint( setup.mEntryPoint );
    SetFilename( setup.mFilename );

    StripBytecodeDebug();

    SaveReflection();
    SaveBytecode();
    SaveDebug(setup.mPDBDir);

    if constexpr( IsDebugMode )
    {
      EnableDebug();
      DisableOptimizations();
    }
    else
    {
      StripBytecodeReflection();
      StripBytecodeRootSignature();
      //StripBytecodePrivateData();
    }
  }

  void DXCArgHelper::AddArgs( StringView arg0, StringView arg1 )
  {
    AddArg(arg0);
    AddArg(arg1);
  }

  void DXCArgHelper::AddArg( StringView arg )
  {
    TAC_NOT_CONST Array args = { arg.data() };
    const HRESULT hr = mArgs->AddArgumentsUTF8( args.data(), args.size() );
    TAC_ASSERT( SUCCEEDED( hr ) );
  }

  void DXCArgHelper::SetFilename( String s )
  {
    const Filesystem::Path fsPath = s;
    auto ext = fsPath.extension();
    TAC_ASSERT( fsPath.has_extension() && ext == ".hlsl" );
    TAC_ASSERT( !fsPath.has_parent_path() );
    TAC_ASSERT( fsPath.has_stem() );
    TAC_ASSERT( mFilename.empty() && mStem.empty() );

    mFilename = s;
    mStem = fsPath.stem().u8string();
    TAC_ASSERT( IsAscii( mStem ) );
    TAC_ASSERT(!mStem.empty());
  }

  void DXCArgHelper::SaveReflection()
  {
    TAC_ASSERT(!mStem.empty());
    AddArgs( "-Fre", mStem + ".refl" );
  }

  void DXCArgHelper::SaveBytecode()  
  {
    TAC_ASSERT(!mStem.empty());
    AddArgs( "-Fo", mStem + ".dxo");
  } // assert path ends in .dxo (it is a dxil file)

  // https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/
  // Best practice is to let dxc name the shader with the hash
  void DXCArgHelper::SaveDebug( const Filesystem::Path& pdbDir )
  {
    TAC_ASSERT( Filesystem::IsDirectory( pdbDir ) );
    String dir = pdbDir.u8string();
    if( !dir.ends_with( "/" ) ||
        !dir.ends_with( "\\" ) )
      dir += '\\'; // ensure trailing slash

    AddArgs( "-Fd",  dir  ); // not quoted
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
  static Filesystem::Path FileStemExtToName( const StringView stem,
                                           const StringView ext )
  {
    TAC_ASSERT( !ext.contains( '.' ) );
    return stem + "." + ext;
  }

  static Filesystem::Path FilenameToPath( StringView filename )
  {
    return sShellPrefPath.Get() / filename.c_str();
  }

  static void SaveBlobToFile( TAC_NOT_CONST PCom< IDxcBlob> blob,
                              const Filesystem::Path& path,
                              Errors& errors )
  {
    const void* bytes = blob->GetBufferPointer();
    const int byteCount = ( int )blob->GetBufferSize();
    TAC_CALL( Filesystem::SaveToFile, path, bytes, byteCount, errors );
  }

  static String GetBlob16AsUTF8( PCom< IDxcBlobUtf16> blob16, PCom<IDxcUtils> pUtils )
  {
    if( !blob16 )
      return {};
    PCom< IDxcBlobUtf8> pName8;
    TAC_ASSERT( SUCCEEDED( pUtils->GetBlobAsUtf8( ( IDxcBlob* )blob16, pName8.CreateAddress() ) ) );
    return String( pName8->GetStringPointer(),
                  ( int )pName8->GetStringLength() );
  }

  static bool DidCompileSucceed( PCom<IDxcResult> pResults )
  {
    HRESULT hrStatus;
    TAC_ASSERT( SUCCEEDED( pResults->GetStatus( &hrStatus ) ) );
    return SUCCEEDED( hrStatus );
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
    const String inputShaderName =  input.mShaderAssetPath.GetFilename();
    const Filesystem::Path hlslShaderPath = sShellPrefPath / inputShaderName;

    TAC_CALL_RET( {}, Filesystem::SaveToFile, hlslShaderPath, input.mPreprocessedShader , errors );

    TAC_NOT_CONST DXCArgHelper::BasicSetup argHelperSetup
    {
      .mEntryPoint = input.mEntryPoint ,
      .mTargetProfile = target,
      .mFilename = inputShaderName,
      .mPDBDir = sShellPrefPath,
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
        if( pErrors )
          errorStr += StringView( pErrors->GetStringPointer(),
                                  ( int )pErrors->GetStringLength() );

      }



      TAC_RAISE_ERROR_RETURN( errorStr, {} );
    }

    PCom<IDxcBlob> pShader;
    if( pResults->HasOutput( DXC_OUT_OBJECT ) )
    {
      //
      // Save shader binary.
      //
      PCom< IDxcBlobUtf16> pShaderName;
      TAC_DX12_CALL_RET( {},
                         pResults->GetOutput,
                         DXC_OUT_OBJECT,
                         pShader.iid(),
                         pShader.ppv(),
                         pShaderName.CreateAddress() );
      TAC_RAISE_ERROR_IF_RETURN( !pShader, "No shader dxil", {} );
      const String outputShaderName = GetBlob16AsUTF8( pShaderName, pUtils );
      const Filesystem::Path dxilShaderPath = sShellPrefPath / outputShaderName;
      TAC_CALL_RET( {}, SaveBlobToFile,pShader, dxilShaderPath, errors );
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
      pResults->GetOutput( DXC_OUT_PDB,
                           pPDB.iid(),
                           pPDB.ppv(),
                           pPDBName.CreateAddress() );
      TAC_RAISE_ERROR_IF_RETURN( !pShader, "No shader pdb", {} );
      const String pdbName = GetBlob16AsUTF8( pPDBName, pUtils );
      const Filesystem::Path pdbPath = sShellPrefPath / pdbName;
      TAC_CALL_RET( {}, SaveBlobToFile,pPDB, pdbPath, errors );
    }

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
