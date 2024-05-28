#if 0
#include "tac_dx12_tutorial_2_dxc.h" // self-inc

#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/string/tac_string_util.h" // IsAscii
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-dx/dx12/tac_dx12_helper.h" // TAC_DX12_CALL_RET

// d3d12 must be included before dxcapi
//#include <d3d12.h> // D3D12_SHADER_BYTECODE
//#include <dxcapi.h> // IDxcUtils, IDxcCompiler3, DxcCreateInstance, 
#pragma comment (lib, "dxcompiler.lib" )

namespace Tac::Render::DXC
{
  // -----------------------------------------------------------------------------------------------

  // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
  struct ExampleDXCArgHelper
  {
    struct BasicSetup
    {
      StringView       mEntryPoint;
      StringView       mTargetProfile;
      StringView       mFilename;
      Filesystem::Path mPDBDir;
      PCom<IDxcUtils>  mUtils;
    };

    ExampleDXCArgHelper( BasicSetup );

    void SetEntryPoint( String s )         { AddArgs( "-E", s ); }
    void SetTargetProfile( String s )      { AddArgs( "-T", s ); }
    void SetFilename( String );
    void SetHLSLVersion( StringView ver = "2021" );
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

  std::wstring ExampleDXCArgHelper::ToWStr(StringView s)
  {
    std::wstring result;
    for( char c : s )
      result += c;
    return result;
  }

  ExampleDXCArgHelper::ExampleDXCArgHelper( BasicSetup setup )
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

    SetHLSLVersion();

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

  void ExampleDXCArgHelper::AddArgs( StringView arg0, StringView arg1 )
  {
    AddArg(arg0);
    AddArg(arg1);
  }

  void ExampleDXCArgHelper::AddArg( StringView arg )
  {
    TAC_NOT_CONST Array args = { arg.data() };
    const HRESULT hr = mArgs->AddArgumentsUTF8( args.data(), args.size() );
    TAC_ASSERT( SUCCEEDED( hr ) );
  }

  void ExampleDXCArgHelper::SetFilename( String s )
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

  void ExampleDXCArgHelper::SetHLSLVersion( StringView ver )
  {
    AddArgs( "-HV", ver );
  }

  void ExampleDXCArgHelper::SaveReflection()
  {
    TAC_ASSERT(!mStem.empty());
    AddArgs( "-Fre", mStem + ".refl" );
  }

  void ExampleDXCArgHelper::SaveBytecode()  
  {
    TAC_ASSERT(!mStem.empty());
    AddArgs( "-Fo", mStem + ".dxo");
  } // assert path ends in .dxo (it is a dxil file)

  // https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/
  // Best practice is to let dxc name the shader with the hash
  void ExampleDXCArgHelper::SaveDebug( const Filesystem::Path& pdbDir )
  {
    TAC_ASSERT( Filesystem::IsDirectory( pdbDir ) );
    String dir = pdbDir.u8string();
    if( !dir.ends_with( "/" ) ||
        !dir.ends_with( "\\" ) )
      dir += '\\'; // ensure trailing slash

    AddArgs( "-Fd",  dir  ); // not quoted
  }

  // -----------------------------------------------------------------------------------------------

  static const char* GetTargetPrefix( ShaderType type )
  {
    switch( type )
    {
    case ShaderType::Vertex: return "vs";
    case ShaderType::Fragment: return "ps";
    case ShaderType::Geometry: return "gs";
    case ShaderType::Compute: return "cs";
    default: TAC_ASSERT_INVALID_CASE( type ); return "";
    }
  }

  static String GetTarget( ShaderType type, D3D_SHADER_MODEL model )
  {
    const char* prefix = GetTargetPrefix( type );
    TAC_ASSERT_INDEX( type, ShaderType::Count );
    return String() + prefix 
      + "_" + ( '0' + ( char )( ( int )model / 16 ) )
      + "_" + ( '0' + ( char )( ( int )model % 16 ) );
  }

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

  // -----------------------------------------------------------------------------------------------

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

      char* ver { nullptr };
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

  PCom<IDxcBlob> ExampleCompile( const ExampleInput& input, Errors& errors )
  {
    TAC_ASSERT( !input.mOutputDir.empty() );

    TAC_ASSERT_MSG(
      input.mShaderModel >= D3D_SHADER_MODEL_6_0,
      "Specifically using dxc instead of d3dcompiler to support a newer shader model" );

    // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

    PCom<IDxcCompiler3> pCompiler;
    TAC_DX12_CALL_RET( {}, DxcCreateInstance( CLSID_DxcCompiler, pCompiler.iid(), pCompiler.ppv() ) );

    PCom<IDxcUtils> pUtils;
    TAC_DX12_CALL_RET( {}, DxcCreateInstance( CLSID_DxcUtils, pUtils.iid(), pUtils.ppv() ) );

    PCom<IDxcPdbUtils> pdbUtils;
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

      const UINT32 n { pResults->GetNumOutputs() };
      for( UINT32 i {}; i < n; ++i )
      {
        DXC_OUT_KIND kind { pResults->GetOutputByIndex( i ) };
        ++asdf;
      }
      ++asdf;
    }

    const String target = GetTarget( input.mType, input.mShaderModel );
    const String inputShaderName =  input.mShaderAssetPath.GetFilename();
    const Filesystem::Path hlslShaderPath = input.mOutputDir / inputShaderName;

    TAC_CALL_RET( {}, Filesystem::SaveToFile( hlslShaderPath, input.mPreprocessedShader, errors ) );

    TAC_NOT_CONST ExampleDXCArgHelper::BasicSetup argHelperSetup
    {
      .mEntryPoint { input.mEntryPoint },
      .mTargetProfile { target },
      .mFilename { inputShaderName },
      .mPDBDir { input.mOutputDir },
      .mUtils { pUtils },
    };
    TAC_NOT_CONST ExampleDXCArgHelper argHelper( argHelperSetup );

    const auto pArguments { argHelper.GetArgs() };
    const auto argCount { argHelper.GetArgCount() };

    const DxcBuffer Source
    {
      .Ptr { input.mPreprocessedShader.data() },
      .Size { ( SIZE_T )input.mPreprocessedShader.size() },
      .Encoding { DXC_CP_ACP },
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

    PCom<IDxcBlob> pShader;
    if( pResults->HasOutput( DXC_OUT_OBJECT ) )
    {
      //
      // Save shader binary.
      //
      PCom< IDxcBlobUtf16> pShaderName;
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
} // namespace Tac::Render::DXC

#endif
