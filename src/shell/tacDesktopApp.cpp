#include "src/common/containers/tacRingBuffer.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacControllerInput.h"
#include "src/common/tacKeyboardInput.h"
#include "src/common/tacOS.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/containers/tacFrameVector.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowManager.h"

namespace Tac
{

  thread_local ThreadType gThreadType = ThreadType::Unknown;

  DesktopApp* DesktopApp::Instance = nullptr;

  DesktopApp::DesktopApp()
  {
    Instance = this;
    TAC_NEW Shell;
  }

  DesktopApp::~DesktopApp()
  {
    //for( auto window : mMainWindows )
    //  delete window;
  }

  static RingBuffer sAllocatorStuff;
  static RingBuffer sAllocatorMain;

  static void DontMaxOutCpuGpuPowerUsage()
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }

  //static void StuffThread( void* userData )
  static void StuffThread()
  {
    Errors& errors = DesktopApp::Instance->mErrorsStuffThread;

    gThreadType = ThreadType::Stuff;

    sAllocatorStuff.Init( 1024 * 1024 * 10 );
    FrameMemory::SetThreadAllocator( &sAllocatorStuff );

    TAC_NEW ProfileSystem;
    ProfileSystem::Instance->Init();

    Render::Init();

    TAC_NEW FontStuff;
    FontStuff::Instance->Load( errors );
    TAC_HANDLE_ERROR( errors );

    DesktopApp::Instance->CreateControllerInput( errors );
    TAC_HANDLE_ERROR( errors );

    while( !OS::mShouldStopRunning )
    {
      Shell::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );

      UpdateThing::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );

      DontMaxOutCpuGpuPowerUsage();
    }
  }

  static void MainThread()
  {
    Errors& errors = DesktopApp::Instance->mErrorsMainThread;
    while( !OS::mShouldStopRunning )
    {

      DesktopApp::Instance->Poll( errors );
      TAC_HANDLE_ERROR( errors );


      DesktopWindowManager::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );

      //KillDeadWindows();

      //Renderer::Instance->Render( errors );
      Render::RenderFrame( errors );
      TAC_HANDLE_ERROR( errors );

      DontMaxOutCpuGpuPowerUsage();
    }

  }

  //void DesktopApp::KillDeadWindows()
  //{
  //  int windowCount = mMainWindows.size();
  //  int iWindow = 0;
  //  while( iWindow < windowCount )
  //  {
  //    DesktopWindow* window = mMainWindows[ iWindow ];
  //    if( window->mRequestDeletion )
  //    {
  //      mMainWindows[ iWindow ] = mMainWindows[ windowCount - 1 ];
  //      delete window;
  //      --windowCount;
  //      mMainWindows.pop_back();
  //    }
  //    else
  //    {
  //      ++iWindow;
  //    }
  //  }
  //}

  void DesktopApp::Init( Errors& errors )
  {
    gThreadType = ThreadType::Main;
    sAllocatorMain.Init( 1024 * 1024 * 10 );
    FrameMemory::SetThreadAllocator( &sAllocatorMain );

    TAC_NEW DesktopWindowManager;

    ExecutableStartupInfo info;
    info.Init( errors );
    TAC_HANDLE_ERROR( errors );

    String appDataPath;
    bool appDataPathExists;
    OS::GetApplicationDataPath( appDataPath, errors );
    OS::DoesFolderExist( appDataPath, appDataPathExists, errors );
    TAC_ASSERT( appDataPathExists );

    String appName = info.mAppName;
    String studioPath = appDataPath + "\\" + info.mStudioName + "\\";
    String prefPath = studioPath + appName;

    OS::CreateFolderIfNotExist( studioPath, errors );
    TAC_HANDLE_ERROR( errors );

    OS::CreateFolderIfNotExist( prefPath, errors );
    TAC_HANDLE_ERROR( errors );

    String workingDir;
    OS::GetWorkingDir( workingDir, errors );
    TAC_HANDLE_ERROR( errors );

    TAC_NEW Shell;
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

    std::thread threads[] =
    {
      std::thread( StuffThread ),
    };

    MainThread();

    for( std::thread& thread : threads )
      thread.join();
  }

  DesktopWindow* DesktopApp::FindDesktopWindow( DesktopWindowHandle desktopWindowHandle )
  {
    for( int i = 0; i < kMaxDesktopWindowStateCount; ++i )
      if( mDesktopWindows[ i ] && mDesktopWindows[ i ]->mHandle == desktopWindowHandle )
        return mDesktopWindows[ i ];
    return nullptr;
  }

  void DesktopApp::SpawnWindow( DesktopWindow* desktopWindow )
  {
    for( int i = 0; i < kMaxDesktopWindowStateCount; ++i )
    {
      if( mDesktopWindows[ i ] )
        continue;
      mDesktopWindows[ i ] = desktopWindow;
      return;
    }
  }


  //void DesktopApp::SpawnWindow( int x, int y, int width, int height )
  //{
  //  DesktopWindow* desktopWindow;
  //  SpawnWindowAux( windowParams, &desktopWindow, errors );
  //  TAC_HANDLE_ERROR( errors );

    //Renderer::Instance->CreateWindowContext( desktopWindow, errors );

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
    //auto onWindowResize = TAC_NEW OnWindowResize;
    //onWindowResize->mDesktopWindow = desktopWindow;
    //onWindowResize->mRendererWindowData = desktopWindow->mRendererData;

    //desktopWindow->mOnResize.AddCallbackFunctional( [ desktopWindow, &errors ]()
    //                                                {
    //                                                  desktopWindow->mRendererData->OnResize( errors );
    //                                                } );


    //TAC_HANDLE_ERROR( errors );

    //mMainWindows.push_back( desktopWindow );
  //}

  //DesktopWindow* DesktopApp::FindWindow( StringView windowName )
  //{
  //  for( DesktopWindow* window : mMainWindows )
  //    if( window->mName == windowName )
  //      return window;
  //  return nullptr;


  //}

  //static WindowState gWindowStates[ 10 ];


}
