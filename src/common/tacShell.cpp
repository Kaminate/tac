#include "common/assetmanagers/tacModelAssetManager.h"
#include "common/assetmanagers/tacTextureAssetManager.h"
#include "common/graphics/tacFont.h"
#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacDebug3D.h"
#include "common/graphics/tacUI2D.h"
#include "common/tacAlgorithm.h"
#include "common/tacJobQueue.h"
#include "common/tacLog.h"
#include "common/tacNet.h"
#include "common/tacOS.h"
#include "common/tacPreprocessor.h"
#include "common/tacSettings.h"
#include "common/tacShell.h"
#include "common/tacTime.h"
#include "common/taccontrollerinput.h"
#include "common/tackeyboardinput.h"
#include <iostream>

const TacKey TacToggleMainMenuKey = TacKey::Backtick;

TacSoul::TacSoul()
{
  mIsImGuiVisible = true;
}

TacShell::TacShell()
{
  mTimer = new TacTimer();
  mKeyboardInput = new TacKeyboardInput();
  mTimer->Start();
  if( TacIsDebugMode() )
    mLog = new TacLog();
}
TacShell::~TacShell()
{
  delete mUI2DCommonData;
  delete mDebug3DCommonData;
  delete mLocalization;
  delete mFontStuff;
  delete mLog;
  delete mNet;
  delete mTimer;
  delete mControllerInput;
  delete mModelAssetManager;
  delete mTextureAssetManager;

  // last, so resources can be freed
  delete mRenderer;
}
void TacShell::Init( TacErrors& errors )
{
  // load settings
  {
    TacString settingsFilename = mAppName + "Settings.txt";
    auto settings = new TacSettings();
    settings->mPath = mPrefPath + "/" + settingsFilename;
    settings->Load( errors );
    TAC_HANDLE_ERROR( errors );
    mSettings = settings;
  }

  // create renderer
  {
    TacVector< TacRendererFactory* >& rendererFactories = TacRendererFactory::GetRegistry();
    if( rendererFactories.empty() )
    {
      errors = "No renderers available";
      return;
    }


    TacRendererFactory* rendererFactory = rendererFactories[ 0 ];
    TacString defaultRendererName = TacOS::Instance->GetDefaultRendererName();
    if( !defaultRendererName.empty() )
    {
      for( TacRendererFactory* curRendererFactory : rendererFactories )
      {
        if( curRendererFactory->mRendererName == defaultRendererName )
        {
          rendererFactory = curRendererFactory;
        }
      }
    }


    TacString rendererName = rendererFactory->mRendererName;
    rendererName = mSettings->GetString( nullptr, { "DefaultRenderer" }, rendererName, errors );
    TAC_HANDLE_ERROR( errors );
    if( !TacFindIf( &rendererFactory, rendererFactories, [ & ]( TacRendererFactory* fact ) { return fact->mRendererName == rendererName; } ) )
      std::cout << "Failed to find " + rendererName + " renderer";
    TacRenderer* renderer = nullptr;
    rendererFactory->CreateRendererOuter( &renderer );
    renderer->mShell = this;
    renderer->Init( errors );
    mRenderer = renderer;
  }

  mJobQueue = new TacJobQueue;
  mJobQueue->Init();

  mTextureAssetManager = new TacTextureAssetManager;
  mTextureAssetManager->mJobQueue = mJobQueue;
  mTextureAssetManager->mRenderer = mRenderer;

  mModelAssetManager = new TacModelAssetManager;
  mModelAssetManager->mJobQueue = mJobQueue;
  mModelAssetManager->mRenderer = mRenderer;

  mFontStuff = new TacFontStuff();
  mFontStuff->Load( mSettings, mRenderer, TacFontAtlasDefaultVramByteCount, errors );
  TAC_HANDLE_ERROR( errors );

  mLocalization = new TacLocalization();
  mLocalization->Load( "assets/localization.txt", errors );
  TAC_HANDLE_ERROR( errors );

  mDebug3DCommonData = new TacDebug3DCommonData;
  mDebug3DCommonData->mRenderer = mRenderer;
  mDebug3DCommonData->Init( errors );
  TAC_HANDLE_ERROR( errors );

  mUI2DCommonData = new TacUI2DCommonData;
  mUI2DCommonData->mRenderer = mRenderer;
  mUI2DCommonData->mFontStuff = mFontStuff;
  mUI2DCommonData->Init( errors );
  TAC_HANDLE_ERROR( errors );
}
void TacShell::FrameBegin( TacErrors& errors )
{
  mKeyboardInput->BeginFrame();
}
void TacShell::Frame( TacErrors& errors )
{
  if( mNet )
  {
    mNet->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  mOnUpdate.EmitEvent();

  if( mControllerInput )
    mControllerInput->Update();
}
void TacShell::FrameEnd( TacErrors& errors )
{
  if( mRenderer )
  {
    mRenderer->Render( errors );
    TAC_HANDLE_ERROR( errors );
  }
  mKeyboardInput->EndFrame();
}
void TacShell::Update( TacErrors& errors )
{
  mTimer->Tick();
  if( mTimer->mAccumulatedSeconds < TAC_DELTA_FRAME_SECONDS )
    return;
  mTimer->mAccumulatedSeconds -= TAC_DELTA_FRAME_SECONDS;
  mElapsedSeconds += TAC_DELTA_FRAME_SECONDS;

  FrameBegin( errors );
  Frame( errors );
  FrameEnd( errors );
}
