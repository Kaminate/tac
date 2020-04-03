#include "shell/tacDesktopApp.h"
#include "shell/tacDesktopWindowManager.h"

#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacUI2D.h"
#include "common/graphics/tacUI.h"
#include "common/tacOS.h"
#include "common/tackeyboardinput.h" // temp
#include "common/profile/tacProfile.h"


TacDesktopApp* TacDesktopApp::Instance = nullptr;

TacDesktopApp::TacDesktopApp()
{
  Instance = this;
  new TacShell;
}

TacDesktopApp::~TacDesktopApp()
{
  for( auto window : mMainWindows )
    delete window;
}

static void StuffThread( TacErrors& errors )
{
  new TacProfileSystem;
  TacProfileSystem::Instance->Init();
  while( !TacOS::Instance->mShouldStopRunning )
  {
    TacShell::Instance->Update( errors );
    TAC_HANDLE_ERROR( errors );

    TacUpdateThing::Instance->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }
}

void TacDesktopApp::KillDeadWindows()
{
  int windowCount = mMainWindows.size();
  int iWindow = 0;
  while( iWindow < windowCount )
  {
    TacDesktopWindow* window = mMainWindows[ iWindow ];
    if( window->mRequestDeletion )
    {
      mMainWindows[ iWindow ] = mMainWindows[ windowCount - 1 ];
      delete window;
      --windowCount;
      mMainWindows.pop_back();
    }
    else
    {
      ++iWindow;
    }
  }
}

void TacDesktopApp::Init( TacErrors& errors )
{
  new TacDesktopWindowManager;

  TacExecutableStartupInfo info;
  info.Init( errors );
  TAC_HANDLE_ERROR( errors );

  TacString appDataPath;
  bool appDataPathExists;
  TacOS::Instance->GetApplicationDataPath( appDataPath, errors );
  TacOS::Instance->DoesFolderExist( appDataPath, appDataPathExists, errors );
  TacAssert( appDataPathExists );

  TacString appName = info.mAppName;
  TacString studioPath = appDataPath + "\\" + info.mStudioName + "\\";
  TacString prefPath = studioPath + appName;

  TacOS::Instance->CreateFolderIfNotExist( studioPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacOS::Instance->CreateFolderIfNotExist( prefPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacString workingDir;
  TacOS::Instance->GetWorkingDir( workingDir, errors );
  TAC_HANDLE_ERROR( errors );

  new TacShell;
  TacShell::Instance->mAppName = appName;
  TacShell::Instance->mPrefPath = prefPath;
  TacShell::Instance->mInitialWorkingDir = workingDir;
  TacShell::Instance->Init( errors );
  TAC_HANDLE_ERROR( errors );

  TacUpdateThing::Instance->Init( errors );
  TAC_HANDLE_ERROR( errors );

}
void TacDesktopApp::Run()
{
  TacErrors& errors = mErrorsMainThread;

  Init( errors );
  TAC_HANDLE_ERROR( errors );

  mStuffThread = std::thread( StuffThread, mErrorsStuffThread );
  for( ;; )
  {
    if( TacOS::Instance->mShouldStopRunning )
      break;

    Poll( errors );
    TAC_HANDLE_ERROR( errors );

    KillDeadWindows();

    TacRenderer::Instance->Render( errors );
    TAC_HANDLE_ERROR( errors );
  }
}

void TacDesktopApp::SpawnWindow( const TacWindowParams& windowParams, TacDesktopWindow** ppDesktopWindow, TacErrors& errors )
{
  TacDesktopWindow* desktopWindow;
  SpawnWindowAux( windowParams, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  TacRenderer::Instance->CreateWindowContext( desktopWindow, errors );

  //struct TacOnWindowResize : public TacEvent<>::Handler
  //{
  //  void HandleEvent() override
  //  {
  //    TacErrors errors;
  //    mRendererWindowData->OnResize( errors );
  //  }
  //  TacRendererWindowData* mRendererWindowData = nullptr;
  //  TacDesktopWindow* mDesktopWindow = nullptr;
  //};
  //auto onWindowResize = new TacOnWindowResize;
  //onWindowResize->mDesktopWindow = desktopWindow;
  //onWindowResize->mRendererWindowData = desktopWindow->mRendererData;

  desktopWindow->mOnResize.AddCallbackFunctional( [ desktopWindow, &errors ]()
                                                  {
                                                    desktopWindow->mRendererData->OnResize( errors );
                                                  } );


  TAC_HANDLE_ERROR( errors );

  mMainWindows.push_back( desktopWindow );

  *ppDesktopWindow = desktopWindow;
}

