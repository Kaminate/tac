#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowManager.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/tacOS.h"
#include "src/common/tacKeyboardInput.h" // temp
#include "src/common/profile/tacProfile.h"

namespace Tac
{

  thread_local ThreadType gThreadType = ThreadType::Unknown;

  DesktopApp* DesktopApp::Instance = nullptr;

  DesktopApp::DesktopApp()
  {
    Instance = this;
    new Shell;
  }

  DesktopApp::~DesktopApp()
  {
    for( auto window : mMainWindows )
      delete window;
  }

  static void StuffThread( Errors& errors )
  {
    gThreadType = ThreadType::Stuff;
    new ProfileSystem;
    ProfileSystem::Instance->Init();
    while( !OS::Instance->mShouldStopRunning )
    {
      Shell::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );

      UpdateThing::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );
    }
  }

  void DesktopApp::KillDeadWindows()
  {
    int windowCount = mMainWindows.size();
    int iWindow = 0;
    while( iWindow < windowCount )
    {
      DesktopWindow* window = mMainWindows[ iWindow ];
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

  void DesktopApp::Init( Errors& errors )
  {
    gThreadType = ThreadType::Main;
    new DesktopWindowManager;

    ExecutableStartupInfo info;
    info.Init( errors );
    TAC_HANDLE_ERROR( errors );

    String appDataPath;
    bool appDataPathExists;
    OS::Instance->GetApplicationDataPath( appDataPath, errors );
    OS::Instance->DoesFolderExist( appDataPath, appDataPathExists, errors );
    TAC_ASSERT( appDataPathExists );

    String appName = info.mAppName;
    String studioPath = appDataPath + "\\" + info.mStudioName + "\\";
    String prefPath = studioPath + appName;

    OS::Instance->CreateFolderIfNotExist( studioPath, errors );
    TAC_HANDLE_ERROR( errors );

    OS::Instance->CreateFolderIfNotExist( prefPath, errors );
    TAC_HANDLE_ERROR( errors );

    String workingDir;
    OS::Instance->GetWorkingDir( workingDir, errors );
    TAC_HANDLE_ERROR( errors );

    new Shell;
    Shell::Instance->mAppName = appName;
    Shell::Instance->mPrefPath = prefPath;
    Shell::Instance->mInitialWorkingDir = workingDir;
    Shell::Instance->Init( errors );
    TAC_HANDLE_ERROR( errors );

    UpdateThing::Instance->Init( errors );
    TAC_HANDLE_ERROR( errors );

  }
  void DesktopApp::Run()
  {
    Errors& errors = mErrorsMainThread;

    Init( errors );
    TAC_HANDLE_ERROR( errors );

    mStuffThread = std::thread( StuffThread, mErrorsStuffThread );
    for( ;; )
    {
      if( OS::Instance->mShouldStopRunning )
        break;

      Poll( errors );
      TAC_HANDLE_ERROR( errors );


      DesktopWindowManager::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );

      KillDeadWindows();

      Renderer::Instance->Render( errors );
      TAC_HANDLE_ERROR( errors );
    }
  }

  void DesktopApp::SpawnWindow( const WindowParams& windowParams, DesktopWindow** ppDesktopWindow, Errors& errors )
  {
    DesktopWindow* desktopWindow;
    SpawnWindowAux( windowParams, &desktopWindow, errors );
    TAC_HANDLE_ERROR( errors );

    Renderer::Instance->CreateWindowContext( desktopWindow, errors );

    //struct OnWindowResize : public Event<>::Handler
    //{
    //  void HandleEvent() override
    //  {
    //    Errors errors;
    //    mRendererWindowData->OnResize( errors );
    //  }
    //  RendererWindowData* mRendererWindowData = nullptr;
    //  DesktopWindow* mDesktopWindow = nullptr;
    //};
    //auto onWindowResize = new OnWindowResize;
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

  DesktopWindow* DesktopApp::FindWindow( StringView windowName )
  {
    for( DesktopWindow* window : mMainWindows )
      if( window->mName == windowName )
        return window;
    return nullptr;


  }
}
