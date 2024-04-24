#include "tac_dxc_arg_helper.h" // self-inc

#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/string/tac_string_util.h" // IsAscii

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

  void DXCArgHelper::SetEntryPoint( StringView s ) { AddArgs( "-E", s ); }
  void DXCArgHelper::SetTargetProfile( StringView s )  { AddArgs( "-T", s ); }
  void DXCArgHelper::DefineMacro( StringView s )       { AddArgs( "-D", s ); }
  void DXCArgHelper::ColPackMtxs()                     { AddArg( "-Zpc" ); }
  void DXCArgHelper::RowPackMtxs()                     { AddArg( "-Zpr" ); }
  void DXCArgHelper::DisableOptimizations()            { AddArg( "-Od" ); }
  void DXCArgHelper::EnableDebug()                     { AddArg( "-Zi" ); } // enable pdb
  void DXCArgHelper::StripBytecodeDebug()              { AddArg( "-Qstrip_debug" ); }
  void DXCArgHelper::StripBytecodeReflection()         { AddArg( "-Qstrip_reflect" ); }
  void DXCArgHelper::StripBytecodeRootSignature()      { AddArg( "-Qstrip_rootsignature" ); }
  LPCWSTR* DXCArgHelper::GetArgs()                     { return mArgs->GetArguments(); }
  UINT32   DXCArgHelper::GetArgCount()                 { return mArgs->GetCount(); }


  DXCArgHelper::DXCArgHelper( Params setup )
  {
    TAC_ASSERT(!setup.mFilename.empty());

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
    TAC_NOT_CONST Array args  { arg.data() };
    const HRESULT hr { mArgs->AddArgumentsUTF8( args.data(), args.size() ) };
    TAC_ASSERT( SUCCEEDED( hr ) );
  }

  void DXCArgHelper::SetFilename( StringView s )
  {
    const Filesystem::Path fsPath { s };
    auto ext { fsPath.extension() };
    TAC_ASSERT( fsPath.has_extension() && ext == ".hlsl" );
    TAC_ASSERT( !fsPath.has_parent_path() );
    TAC_ASSERT( fsPath.has_stem() );
    TAC_ASSERT( mFilename.empty() && mStem.empty() );

    mFilename = s;
    mStem = fsPath.stem().u8string();
    TAC_ASSERT( IsAscii( mStem ) );
    TAC_ASSERT(!mStem.empty());
  }

  void DXCArgHelper::SetHLSLVersion( StringView ver )
  {
    AddArgs( "-HV", ver );
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
    String dir { pdbDir.u8string() };
    if( !dir.ends_with( "/" ) ||
        !dir.ends_with( "\\" ) )
      dir += '\\'; // ensure trailing slash

    AddArgs( "-Fd",  dir  ); // not quoted
  }


} // namespace Tac::Render

