// The shell contains basically everything needed to run an application,
// except for the logic

#pragma once

#include "tac-engine-core/i18n/tac_localization.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/string/tac_string.h"

namespace Tac::FileSys { struct Path; }
namespace Tac { struct Errors; struct AssetPathStringView; }
namespace Tac
{
  struct Soul
  {
    virtual ~Soul() = default;
    virtual void Init( SettingsNode, Errors& ) = 0;
    virtual void Update( Errors& ) = 0;
    virtual void DebugImgui( Errors& ) = 0;

    // unowned
    bool         mIsImGuiVisible = true;
    Language     mLanguage { Language::English };
  };

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
