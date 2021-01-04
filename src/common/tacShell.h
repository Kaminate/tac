// The shell contains basically everything needed to run an application,
// except for the logic

#pragma once

#include "src/common/tacLocalization.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacTime.h"
#include "src/common/tacEvent.h"

namespace Tac
{
  struct Log;
  struct RenderView;

  struct Soul
  {
    Soul();
    virtual ~Soul() = default;
    virtual void Init( Errors& ) = 0;
    virtual void Update( Errors& ) = 0;
    virtual void DebugImgui( Errors& ) = 0;

    // unowned
    RenderView*  mRenderView = nullptr;
    bool         mIsImGuiVisible;
    Language     mLanguage = Language::English;
  };

  //
  // The shell acts as the interface between platform-specific applications
  // and the ghost
  //
  struct Shell
  {
    static Shell    Instance;
    void            Init( Errors& );
    void            Uninit();
    void            Update( Errors& );
    void            FrameBegin( Errors& );
    void            Frame( Errors& );
    void            FrameEnd( Errors& );
    Log*            mLog = nullptr;
    String          mAppName;

    // This is the directory where files can be written.
    // Unique per user, per application.
    // ( doesn't include a trailing slash )
    String          mPrefPath;

    String          mInitialWorkingDir;
    Vector< Soul* > mSouls;
    double          mElapsedSeconds = 0;
    Timepoint       mLastTick;
    float           mAccumulatorSeconds = 0;
  };
}
