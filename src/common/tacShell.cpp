#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacFont.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacJobQueue.h"
#include "src/common/tacLog.h"
#include "src/common/tacNet.h"
#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacSettings.h"
#include "src/common/tacShell.h"
#include "src/common/tacTime.h"
#include "src/common/tacControllerinput.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/profile/tacProfile.h"
#include <iostream>

namespace Tac
{

UpdateThing::UpdateThing()
{
  Instance = this;
}
UpdateThing* UpdateThing::Instance = nullptr;

const Key ToggleMainMenuKey = Key::Backtick;

Soul::Soul()
{
  mIsImGuiVisible = true;
}

Shell* Shell::Instance = nullptr;
Shell::Shell()
{
  //mTimer = new Timer();
  new KeyboardInput();
  //mTimer->Start();
  if( IsDebugMode() )
    mLog = new Log();
  Instance = this;
  mLastTick = GetCurrentTime();
}
Shell::~Shell()
{
  delete UI2DCommonData::Instance;
  delete Debug3DCommonData::Instance;
  delete Localization::Instance;
  delete FontStuff::Instance;
  delete mLog;
  delete ModelAssetManager::Instance;
  delete TextureAssetManager::Instance;

  // last, so resources can be freed
  delete Renderer::Instance;

  Instance = nullptr;
}
void Shell::Init( Errors& errors )
{
  // load settings
  {
    String settingsFilename = mAppName + "Settings.txt";
    auto settings = new Settings();
    settings->mPath = mPrefPath + "/" + settingsFilename;
    settings->Load( errors );
    TAC_HANDLE_ERROR( errors );
    mSettings = settings;
  }

  // create renderer
  {
    RendererRegistry& registry = RendererRegistry::Instance();
    if( registry.mFactories.empty() )
    {
      errors = "No renderers available";
      TAC_HANDLE_ERROR( errors );
    }

    String defaultRendererName = OS::Instance->GetDefaultRendererName();
    String settingsRendererName = mSettings->GetString( nullptr, { "DefaultRenderer" }, "", errors );
    RendererFactory* settingsRendererFactory = registry.FindFactory( settingsRendererName );
    RendererFactory* defaultOSRendererFactory = registry.FindFactory( defaultRendererName );
    RendererFactory* firstRendererFactory = registry.mFactories[ 0 ];
    for( RendererFactory* factory :
      {
        settingsRendererFactory,
        defaultOSRendererFactory,
        firstRendererFactory
      } )
    {
      if( !factory )
        continue;
      factory->CreateRendererOuter();
      break;
    }

    if( !Renderer::Instance )
    {
      errors = "Failed to create renderer";
      TAC_HANDLE_ERROR( errors );
    }

    Renderer::Instance->Init( errors );
  }

  new JobQueue;
  JobQueue::Instance->Init();

  new TextureAssetManager;

  new ModelAssetManager;

  new Localization;
  Localization::Instance->Load( "assets/localization.txt", errors );
  TAC_HANDLE_ERROR( errors );

  new Debug3DCommonData;
  Debug3DCommonData::Instance->Init( errors );
  TAC_HANDLE_ERROR( errors );

  new UI2DCommonData;
  UI2DCommonData::Instance->Init( errors );
  TAC_HANDLE_ERROR( errors );

  new ProfileSystem;
  ProfileSystem::Instance->Init();
}
void Shell::FrameBegin( Errors& errors )
{
  KeyboardInput::Instance->BeginFrame();
  ProfileSystem::Instance->OnFrameBegin();
}
void Shell::Frame( Errors& errors )
{
  TAC_PROFILE_BLOCK;
  FrameBegin( errors );

  if( Net::Instance )
  {
    Net::Instance->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  mOnUpdate.EmitEvent( errors );
  ControllerInput::Instance->Update();

  FrameEnd( errors );
}
void Shell::FrameEnd( Errors& errors )
{
  KeyboardInput::Instance->EndFrame();
  ProfileSystem::Instance->OnFrameEnd();
}
void Shell::Update( Errors& errors )
{
  Timepoint curTime = GetCurrentTime();
  mAccumulatorSeconds += TimepointSubtractSeconds(curTime, mLastTick);
  mLastTick = curTime;
  if( mAccumulatorSeconds < TAC_DELTA_FRAME_SECONDS )
    return;
  mAccumulatorSeconds -= TAC_DELTA_FRAME_SECONDS;
  mElapsedSeconds += TAC_DELTA_FRAME_SECONDS;

  Frame( errors );
}


RendererWindowData::~RendererWindowData()
{
  static int i;
  ++i;
}
}
