// The shell contains basically everything needed to run an application,
// except for the logic

#pragma once

#include "common/tacLocalization.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/tacPreprocessor.h"
#include "common/containers/tacVector.h"
#include "common/tacEvent.h"

struct TacAssetManager;
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
struct TacTimer;
struct TacUI2DCommonData;

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
  TacShell* mShell = nullptr;
  TacLanguage mLanguage = TacLanguage::English;
};

//
// The shell acts as the interface between platform-specific applications
// and the ghost
//
struct TacShell
{
  TacShell();
  ~TacShell();
  void Update( TacErrors& errors );
  void Init( TacErrors& errors );
  void AddSoul( TacSoul* soul );

  TacAssetManager* mAssetManager = nullptr;
  TacControllerInput* mControllerInput = nullptr;
  TacEvent< const TacString& >::Emitter mLogData;
  TacEvent<>::Emitter mOnUpdate;
  TacFontStuff* mFontStuff = nullptr;
  TacJobQueue* mJobQueue = nullptr;
  TacKeyboardInput* mKeyboardInput = nullptr;
  TacLocalization* mLocalization = nullptr;
  TacLog* mLog = nullptr;
  TacNet* mNet = nullptr;
  TacRenderer* mRenderer = nullptr;
  TacSettings* mSettings = nullptr;
  TacString mAppName;
  // This is the directory where files can be written.
  // Unique per user, per application.
  // ( doesn't include a trailing slash )
  TacString mPrefPath;
  TacTextureAssetManager* mTextureAssetManager = nullptr;
  TacTimer* mTimer = nullptr;
  TacUI2DCommonData* mUI2DCommonData = nullptr;
  TacVector< TacSoul* > mSouls;
  double mElapsedSeconds = 0;
};

struct TacRendererWindowData
{
  virtual ~TacRendererWindowData() = default;
  virtual void Submit( TacErrors& errors ) {};
  virtual void GetCurrentBackbufferTexture( TacTexture** texture ) { TacUnimplemented; };
  virtual void OnResize( TacErrors& errors ) {};

  TacDesktopWindow* mDesktopWindow = nullptr;
  TacRenderer* mRenderer = nullptr;
  TacDepthBuffer* mDepthBuffer = nullptr;
};


