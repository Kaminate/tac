// The shell contains basically everything needed to run an application,
// except for the logic

#pragma once

#include "common/tacLocalization.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/tacPreprocessor.h"
#include "common/containers/tacVector.h"
#include "common/tacTime.h"
#include "common/tacEvent.h"

struct TacControllerInput;
struct TacDepthBuffer;
struct TacDesktopWindow;
struct TacFontStuff;
struct TacGhost;
struct TacJobQueue;
struct TacKeyboardInput;
struct TacLocalization;
struct TacLog;
struct TacNet;
struct TacRenderView;
struct TacRenderer;
struct TacSettings;
struct TacShell;
struct TacSoul;
struct TacTexture;
struct TacTextureAssetManager;
struct TacModelAssetManager;
struct TacTimer;
struct TacUI2DCommonData;
struct TacDebug3DCommonData;

struct TacSoul
{
  TacSoul();
  virtual ~TacSoul() = default;
  virtual void Init( TacErrors& errors ) = 0;
  virtual void Update( TacErrors& errors ) = 0;
  virtual void DebugImgui( TacErrors& errors ) = 0;

  // unowned
  TacRenderView* mRenderView = nullptr;
  bool mIsImGuiVisible;
  TacLanguage mLanguage = TacLanguage::English;
};

struct TacUpdateThing
{
  TacUpdateThing();
  static TacUpdateThing* Instance;
  virtual void Init( TacErrors& errors ) = 0;
  virtual void Update( TacErrors& errors ) = 0;
};

struct TacExecutableStartupInfo
{
  void Init( TacErrors& errors );
  TacString mAppName;
  TacString mStudioName;
};

//
// The shell acts as the interface between platform-specific applications
// and the ghost
//
struct TacShell
{
  static TacShell* Instance;
  TacShell();
  ~TacShell();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );

  void FrameBegin( TacErrors& errors );
  void Frame( TacErrors& errors );
  void FrameEnd( TacErrors& errors );


  TacEvent< const TacString& >::Emitter mLogData;
  TacEvent< TacErrors& >::Emitter mOnUpdate;
  TacLog* mLog = nullptr;
  TacSettings* mSettings = nullptr;
  TacString mAppName;
  // This is the directory where files can be written.
  // Unique per user, per application.
  // ( doesn't include a trailing slash )
  TacString mPrefPath;
  TacString mInitialWorkingDir;
  TacTimer* mTimer = nullptr;
  TacVector< TacSoul* > mSouls;
  double mElapsedSeconds = 0;
  TacTimepoint mLastTick;
  float mAccumulatorSeconds = 0;
};

struct TacRendererWindowData
{
  virtual ~TacRendererWindowData();
  virtual void Submit( TacErrors& errors ) {};
  virtual void GetCurrentBackbufferTexture( TacTexture** texture ) { TacUnimplemented; };
  virtual void OnResize( TacErrors& errors ) {};

  TacDesktopWindow* mDesktopWindow = nullptr;
  TacDepthBuffer* mDepthBuffer = nullptr;
};


