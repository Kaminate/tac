// The shell contains basically everything needed to run an application,
// except for the logic

#pragma once

#include "tac-std-lib/i18n/tac_localization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/tac_core.h"

namespace Tac
{
  //struct Log;


  struct Soul
  {
    Soul();
    virtual ~Soul() = default;
    virtual void Init( Errors& ) = 0;
    virtual void Update( Errors& ) = 0;
    virtual void DebugImgui( Errors& ) = 0;

    // unowned
    bool         mIsImGuiVisible;
    Language     mLanguage = Language::English;
  };

  //
  // The shell acts as the interface between platform-specific applications
  // and the ghost
  //
  void                    ShellInit( Errors& );
  void                    ShellUninit();
  extern String           sShellAppName;
  extern String           sShellStudioName;
  extern Filesystem::Path sShellPrefPath;
  extern Filesystem::Path sShellInitialWorkingDir;

  //                      Converts a filesystem path to an asset path
  //
  //                      Try not to use this function, instead prefer using a function that
  //                      returns AssetPaths instead of Filesystem::Path's
  AssetPathStringView     ModifyPathRelative( const Filesystem::Path&, Errors& );

  AssetPathStringView     AssetOpenDialog( Errors& );

  struct AssetSaveDialogParams
  {
    String mSuggestedFilename;
  };

  AssetPathStringView     AssetSaveDialog( const AssetSaveDialogParams&, Errors& );


  //Log*            mLog = nullptr;
  //String          mAppName;

  // This is the directory where files can be written.
  // Unique per user, per application.
  // ( doesn't include a trailing slash )
  //String          mPrefPath;

  //String          mInitialWorkingDir;
  //Vector< Soul* > mSouls;
}
