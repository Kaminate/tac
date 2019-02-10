// The shell contains basically everything needed to run an application,
// except for the logic

#pragma once


#include "common/tacLocalization.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/tacPreprocessor.h"
#include "common/containers/tacVector.h"
#include "common/tacEvent.h"


struct ImGuiContext;
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
struct TacRenderContext;
struct TacRenderer;
struct TacRenderView;
struct TacSettings;
struct TacShell;
struct TacSoul;
struct TacTexture;
struct TacTextureAssetManager;
struct TacTimer;
struct TacUI2DCommonData;
struct TacUI2DDrawData;


//typedef TacSoul*( TacGhostCreateFn )( TacShell* shell, TacErrors& errors );
//#define TAC_GHOST_CREATE GhostCreate

struct TacSoul
{
  TacSoul();
  virtual ~TacSoul() = default;
  virtual void Init( TacErrors& errors ) = 0;
  virtual void Update( TacErrors& errors ) = 0;
  virtual void DebugImgui( TacErrors& errors ) = 0;
  TacString GetDebugName();

  TacRenderView* mRenderView = nullptr;


  int mID = 0;
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
  void SetScopedGlobals();
  void Update( TacErrors& errors );
  void Frame( TacErrors& errors );
  void Init( TacErrors& errors );


  TacEvent<>::Emitter mDebugImguiAux;
  TacEvent<>::Emitter mOnRenderBegin;

  void DebugImgui( TacErrors& errors );
  //void AddSoul( TacErrors& errors );
  void AddSoul( TacSoul* soul );
  ImGuiContext* mImGuiContext = nullptr;
  bool mImGuiRender = true;
  bool mImGuiShowTestWindow = false;
  TacRenderer* mRenderer = nullptr;
  TacRenderContext* mRenderContext = nullptr;
  TacUI2DCommonData* mUI2DCommonData = nullptr;
  TacNet* mNet = nullptr;
  TacKeyboardInput* mKeyboardInput;
  TacControllerInput* mControllerInput = nullptr;
  double mElapsedSeconds = 0;

  TacLog* mLog = nullptr;
  bool mShowMainMenu = false;
  bool mShowShellWindow = false;
  TacString mToStdOut = "Hello World";
  TacVector< TacSoul* > mSouls;
  TacTimer* mTimer = nullptr;
  TacFontStuff* mFontStuff = nullptr;
  TacLocalization* mLocalization = nullptr;

  int mSoulIDCounter = 0;
  bool mPaused = false;

  // This is the directory where files can be written.
  // Unique per user, per application.
  // ( doesn't include a trailing slash )
  TacString mPrefPath;

  TacString mAppName;
  TacEvent< const TacString& >::Emitter mLogData;
  TacEvent<>::Emitter mOnUpdate;

  TacSettings* mSettings;

  TacJobQueue* mJobQueue = nullptr;
  TacAssetManager* mAssetManager = nullptr;
  TacTextureAssetManager* mTextureAssetManager = nullptr;
};

struct TacRendererWindowData
{
  virtual ~TacRendererWindowData() = default;

  // This is comparable to bgfx's RendererContextD3D12::submit
  virtual void Submit( TacErrors& errors ) {};
  virtual void GetCurrentBackbufferTexture( TacTexture** texture ) { TacUnimplemented; };
  virtual void OnResize( TacErrors& errors ) {};

  TacDesktopWindow* mDesktopWindow = nullptr;
  TacRenderer* mRenderer = nullptr;
  TacDepthBuffer* mDepthBuffer = nullptr;
};


