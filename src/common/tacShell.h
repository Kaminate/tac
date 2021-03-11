// The shell contains basically everything needed to run an application,
// except for the logic

#pragma once

#include "src/common/tacLocalization.h"
//#include "src/common/tacErrorHandling.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacTime.h"
//#include "src/common/tacEvent.h"

namespace Tac
{
  struct Log;
  struct Errors;

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
  //struct Shell
  //{
  //  static Shell    Instance;
    void            ShellInit( Errors& );
    void            ShellUninit();
    void            ShellUpdate( Errors& );
    //void            ShellFrameBegin( Errors& );
    void            ShellFrame( Errors& );
    //void            ShellFrameEnd( Errors& );
    void            ShellSetAppName(const char*);
    const char*     ShellGetAppName();
    void            ShellSetPrefPath(const char*);
    const char*     ShellGetPrefPath();
    void            ShellSetInitialWorkingDir(const char*);
    const char*     ShellGetInitialWorkingDir();
    //Log*            mLog = nullptr;
    //String          mAppName;

    // This is the directory where files can be written.
    // Unique per user, per application.
    // ( doesn't include a trailing slash )
    //String          mPrefPath;

    //String          mInitialWorkingDir;
    //Vector< Soul* > mSouls;
}
