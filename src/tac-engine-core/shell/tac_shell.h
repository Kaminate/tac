#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-engine-core/asset/tac_asset.h"
namespace Tac
{
  //
  // The shell acts as the interface between platform-specific applications
  // and the ghost
  //
  struct Shell
  {
    static void Init( Errors& );
    static void Uninit();
    static String           sShellAppName;
    static String           sShellStudioName;
    static FileSys::Path    sShellPrefPath;
    static FileSys::Path    sShellInitialWorkingDir;
  };

  //                      Converts a filesystem path to an asset path
  //
  //                      Try not to use this function, instead prefer using a function that
  //                      returns AssetPaths instead of FileSys::Path's
  auto ModifyPathRelative( const FileSys::Path&, Errors& ) -> AssetPathStringView;
} // namespace Tac
