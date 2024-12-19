#include "tac_dxc_arg_helper.h" // self-inc

#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/string/tac_string_util.h" // IsAscii

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <string>
#endif

namespace Tac::Render
{
  static std::wstring ToWStr( StringView s )
  {
    std::wstring result;
    for( char c : s )
      result += c;
    return result;
  }

  // -----------------------------------------------------------------------------------------------

  DXCArgHelper::DXCArgHelper( Params setup )
  {
    TAC_ASSERT( !setup.mFilename.empty() );

    const HRESULT hr{ setup.mUtils->BuildArguments(
      ToWStr( setup.mFilename ).data(),
      ToWStr( setup.mEntryPoint ).data(),
      ToWStr( setup.mTargetProfile ).data(),
      nullptr,
      0,
      nullptr,
      0,
      mArgs.CreateAddress() ) };
    TAC_ASSERT( SUCCEEDED(hr) );


    //SetTargetProfile(setup.mTargetProfile);
    //SetEntryPoint( setup.mEntryPoint );
    SetFilename( setup.mFilename );

    StripBytecodeDebug();

    SaveReflection();
    SaveBytecode();
    SaveDebug( setup.mPDBDir );

    SetHLSLVersion();

    if constexpr( kIsDebugMode )
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

  void     DXCArgHelper::SetEntryPoint( StringView s )     { AddArgs( "-E", s ); }
  void     DXCArgHelper::SetTargetProfile( StringView s )  { AddArgs( "-T", s ); }
  void     DXCArgHelper::DefineMacro( StringView s )       { AddArgs( "-D", s ); }
  void     DXCArgHelper::ColPackMtxs()                     { AddArg( "-Zpc" ); }
  void     DXCArgHelper::RowPackMtxs()                     { AddArg( "-Zpr" ); }
  void     DXCArgHelper::DisableOptimizations()            { AddArg( "-Od" ); }
  void     DXCArgHelper::EnableDebug()                     { AddArg( "-Zi" ); } // enable pdb
  void     DXCArgHelper::StripBytecodeDebug()              { AddArg( "-Qstrip_debug" ); }
  void     DXCArgHelper::StripBytecodeReflection()         { AddArg( "-Qstrip_reflect" ); }
  void     DXCArgHelper::StripBytecodeRootSignature()      { AddArg( "-Qstrip_rootsignature" ); }
  LPCWSTR* DXCArgHelper::GetArgs()                         { return mArgs->GetArguments(); }
  UINT32   DXCArgHelper::GetArgCount()                     { return mArgs->GetCount(); }

  void     DXCArgHelper::AddArgs( StringView arg0, StringView arg1 )
  {
    AddArg( arg0 );
    AddArg( arg1 );
  }

  void     DXCArgHelper::AddArg( StringView arg )
  {
    dynmc Array args{ arg.data() };
    const HRESULT hr{ mArgs->AddArgumentsUTF8( args.data(), args.size() ) };
    TAC_ASSERT( SUCCEEDED( hr ) );
  }

  void     DXCArgHelper::SetFilename( StringView s )
  {
    const FileSys::Path fsPath{ s };
    const FileSys::Path ext{ fsPath.extension() };
    TAC_ASSERT( fsPath.has_extension() && ext == ".hlsl" );
    TAC_ASSERT( !fsPath.has_parent_path() );
    TAC_ASSERT( fsPath.has_stem() );
    TAC_ASSERT( mFilename.empty() && mStem.empty() );

    mFilename = s;
    mStem = fsPath.stem().u8string();
    TAC_ASSERT( IsAscii( mStem ) );
    TAC_ASSERT( !mStem.empty() );
  }

  void     DXCArgHelper::SetHLSLVersion( StringView ver )  { AddArgs( "-HV", ver ); }

  void     DXCArgHelper::SaveReflection()
  {
    TAC_ASSERT( !mStem.empty() );
    AddArgs( "-Fre", mStem + ".refl" );
  }

  void     DXCArgHelper::SaveBytecode()
  {
    TAC_ASSERT( !mStem.empty() );
    AddArgs( "-Fo", mStem + ".dxo" );
  } // assert path ends in .dxo (it is a dxil file)

  // https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/
  // Best practice is to let dxc name the shader with the hash
  void     DXCArgHelper::SaveDebug( const FileSys::Path& pdbDir )
  {
    TAC_ASSERT( FileSys::IsDirectory( pdbDir ) );
    dynmc String dir{ pdbDir.u8string() };
    if( !dir.ends_with( "/" ) ||
        !dir.ends_with( "\\" ) )
      dir += '\\'; // ensure trailing slash

    AddArgs( "-Fd", dir ); // not quoted

    // | test: trying to see if GPU-BASED VALIDATION (GPV) errors will 
    // | find shader code, ie: Shader Code: <couldn't find file location in debug info>
    // | (didnt work)
    // v
    //AddArgs( "-I", dir ); // Add directory to include search path
  }


} // namespace Tac::Render

