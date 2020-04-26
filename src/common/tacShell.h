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


  struct ControllerInput;
  struct DepthBuffer;
  struct DesktopWindow;
  struct FontStuff;
  struct Ghost;
  struct JobQueue;
  struct KeyboardInput;
  struct Localization;
  struct Log;
  struct Net;
  struct RenderView;
  struct Renderer;
  struct Settings;
  struct Shell;
  struct Soul;
  struct Texture;
  struct ModelAssetManager;
  struct Timer;
  struct UI2DCommonData;
  struct Debug3DCommonData;

  struct Soul
  {
    Soul();
    virtual ~Soul() = default;
    virtual void Init( Errors& errors ) = 0;
    virtual void Update( Errors& errors ) = 0;
    virtual void DebugImgui( Errors& errors ) = 0;

    // unowned
    RenderView* mRenderView = nullptr;
    bool mIsImGuiVisible;
    Language mLanguage = Language::English;
  };

  struct UpdateThing
  {
    UpdateThing();
    static UpdateThing* Instance;
    virtual void Init( Errors& errors ) = 0;
    virtual void Update( Errors& errors ) = 0;
  };

  struct ExecutableStartupInfo
  {
    void Init( Errors& errors );
    String mAppName;
    String mStudioName;
  };

  //
  // The shell acts as the interface between platform-specific applications
  // and the ghost
  //
  struct Shell
  {
    static Shell* Instance;
    Shell();
    ~Shell();
    void Init( Errors& errors );
    void Update( Errors& errors );

    void FrameBegin( Errors& errors );
    void Frame( Errors& errors );
    void FrameEnd( Errors& errors );


    Event< const String& >::Emitter mLogData;
    Event< Errors& >::Emitter mOnUpdate;
    Log* mLog = nullptr;
    Settings* mSettings = nullptr;
    String mAppName;
    // This is the directory where files can be written.
    // Unique per user, per application.
    // ( doesn't include a trailing slash )
    String mPrefPath;
    String mInitialWorkingDir;
    Timer* mTimer = nullptr;
    Vector< Soul* > mSouls;
    double mElapsedSeconds = 0;
    Timepoint mLastTick;
    float mAccumulatorSeconds = 0;
  };

  struct RendererWindowData
  {
    virtual ~RendererWindowData();
    virtual void Submit( Errors& ) {};
    virtual void GetCurrentBackbufferTexture( Texture** ) { TAC_UNIMPLEMENTED; };
    virtual void OnResize( Errors& ) {};

    DesktopWindow* mDesktopWindow = nullptr;
    //DepthBuffer* mDepthBuffer = nullptr;
  };


}
