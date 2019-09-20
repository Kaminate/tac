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
  new TacKeyboardInput();
  mTimer->Start();
  if( TacIsDebugMode() )
    mLog = new TacLog();
}
TacShell::~TacShell()
{
  delete TacUI2DCommonData::Instance;
  delete TacDebug3DCommonData::Instance;
  delete mLocalization;
  delete mFontStuff;
  delete mLog;
  delete mTimer;
  delete TacModelAssetManager::Instance;
  delete TacTextureAssetManager::Instance;

  // last, so resources can be freed
  delete TacRenderer::Instance;
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
    TacVector< TacRendererFactory* >& rendererFactories = TacRendererRegistry::Instance().mFactories;
    if( rendererFactories.empty() )
    {
      errors = "No renderers available";
      TAC_HANDLE_ERROR( errors );
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
    TacRenderer::Instance->mShell = this;
    TacRenderer::Instance->Init( errors );
    TacRenderer::Instance = renderer;
  }

  TacJobQueue::Instance = new TacJobQueue;
  TacJobQueue::Instance->Init();

  TacTextureAssetManager::Instance = new TacTextureAssetManager;

  TacModelAssetManager::Instance = new TacModelAssetManager;

  mFontStuff = new TacFontStuff();
  mFontStuff->Load( mSettings, TacRenderer::Instance, TacFontAtlasDefaultVramByteCount, errors );
  TAC_HANDLE_ERROR( errors );

  mLocalization = new TacLocalization();
  mLocalization->Load( "assets/localization.txt", errors );
  TAC_HANDLE_ERROR( errors );

  TacDebug3DCommonData::Instance = new TacDebug3DCommonData;
  TacDebug3DCommonData::Instance->Init( errors );
  TAC_HANDLE_ERROR( errors );

  TacUI2DCommonData::Instance = new TacUI2DCommonData;
  TacUI2DCommonData::Instance->mFontStuff = mFontStuff;
  TacUI2DCommonData::Instance->Init( errors );
  TAC_HANDLE_ERROR( errors );
}
void TacShell::FrameBegin( TacErrors& errors )
{
  TacKeyboardInput::Instance->BeginFrame();
}
void TacShell::Frame( TacErrors& errors )
{
  if( TacNet::Instance )
  {
    TacNet::Instance->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  mOnUpdate.EmitEvent();

  TacControllerInput::Instance->Update();
}
void TacShell::FrameEnd( TacErrors& errors )
{
  if( TacRenderer::Instance )
  {
    TacRenderer::Instance->Render( errors );
    TAC_HANDLE_ERROR( errors );
  }
  TacKeyboardInput::Instance->EndFrame();
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
