#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance


namespace Tac::Render
{

  // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
  struct DXCArgHelper
  {
    struct Params
    {
      StringView        mEntryPoint;
      StringView        mTargetProfile;
      StringView        mFilename;
      FileSys::Path     mPDBDir;
      PCom< IDxcUtils > mUtils;
    };

    DXCArgHelper( Params );

    void SetEntryPoint( StringView s );
    void SetTargetProfile( StringView s );
    void SetFilename( StringView );
    void SetHLSLVersion( StringView ver = "2021" );
    void DefineMacro( StringView s );
    void ColPackMtxs();
    void RowPackMtxs();
    void DisableOptimizations();
    void EnableDebug();
    void StripBytecodeDebug();
    void StripBytecodeReflection();
    void StripBytecodeRootSignature();
    void SaveReflection();
    void SaveBytecode();
    void SaveDebug( const FileSys::Path& pdbDir );
    void AddArgs( StringView, StringView );
    void AddArg( StringView );
    LPCWSTR* GetArgs();
    UINT32   GetArgCount();


  private:
    PCom< IDxcCompilerArgs > mArgs;
    String                   mFilename;
    String                   mStem;
  };
} // namespace Tac::Render

