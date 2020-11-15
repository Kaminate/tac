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

  //UpdateThing::UpdateThing()
  //{
  //  Instance = this;
  //}
  //UpdateThing* UpdateThing::Instance = nullptr;

  static void CreateRenderer( Errors& )
  {
      const String defaultRendererName = OS::GetDefaultRendererName();
      if( const RendererFactory* factory = RendererFactoriesFind( defaultRendererName ) )
      {
        factory->mCreateRenderer();
        return;
      }

      for( RendererFactory* factory : RendererRegistry() )
      {
        factory->mCreateRenderer();
        return;
      }
  }

  const Key ToggleMainMenuKey = Key::Backtick;

  Soul::Soul()
  {
    mIsImGuiVisible = true;
  }

  Shell Shell::Instance;
  void Shell::Uninit()
  {
    gUI2DCommonData.Uninit();
    gDebug3DCommonData.Uninit();
    gFontStuff.Uninit();
    delete mLog;
    gModelAssetManager.Uninit();

    // last, so resources can be freed
    Render::Uninit();
  }
  void Shell::Init( Errors& errors )
  {
    CreateRenderer(errors);
    Render::Init( errors );
    TAC_HANDLE_ERROR( errors );

    JobQueueInit();

    TAC_NEW ModelAssetManager;

    gLocalization.Load( "assets/localization.txt", errors );
    TAC_HANDLE_ERROR( errors );

    gDebug3DCommonData.Init( errors );
    TAC_HANDLE_ERROR( errors );

    gUI2DCommonData.Init( errors );
    TAC_HANDLE_ERROR( errors );

    TAC_NEW ProfileSystem;
    ProfileSystem::Instance->Init();
  }
  void Shell::FrameBegin( Errors& errors )
  {
    //gKeyboardInput.BeginFrame();
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

    //mOnUpdate.EmitEvent( errors );
    ControllerInput::Instance->Update();

    FrameEnd( errors );
  }
  void Shell::FrameEnd( Errors& errors )
  {
    //gKeyboardInput.EndFrame();
    ProfileSystem::Instance->OnFrameEnd();
  }
  void Shell::Update( Errors& errors )
  {
    Timepoint curTime = GetCurrentTime();
    mAccumulatorSeconds += TimepointSubtractSeconds( curTime, mLastTick );
    mLastTick = curTime;
    if( mAccumulatorSeconds < TAC_DELTA_FRAME_SECONDS )
      return;
    mAccumulatorSeconds -= TAC_DELTA_FRAME_SECONDS;
    mElapsedSeconds += TAC_DELTA_FRAME_SECONDS;

    //OS::mShouldStopRunning =  mElapsedSeconds > 5 ;
    Frame( errors );
  }


  //RendererWindowData::~RendererWindowData()
  //{
  //}
}
