#include "src/common/containers/tacFrameVector.h"
#include "src/common/containers/tacRingBuffer.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacControllerInput.h"
#include "src/common/tacDesktopWindow.h" // why is this in common
#include "src/common/tacFrameMemory.h"
#include "src/common/tacKeyboardInput.h"
#include "src/common/tacOS.h"
#include "src/common/tacSettings.h"
#include "src/common/tacString.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowManager.h"


#include <mutex>

namespace Tac
{

  Errors               sPlatformThreadErrors;
  Errors               sLogicThreadErrors;
  AppInterfacePlatform sAppInterfacePlatform;
  AppInterfaceProject  sAppInterfaceProject;

  enum class DesktopEventType
  {
    Unknown = 0,
    WindowCreate,
    WindowResize,
    WindowMove,
    CursorUnobscured,
    KeyState,
    KeyInput,
    MouseWheel,
    MouseMove,
  };
  struct DesktopEventQueueImpl
  {
    void Init();
    void QueuePush( DesktopEventType, void*, int );
    bool QueuePop( void*, int );
    bool Empty();
    RingBuffer mQueue;
    std::mutex mMutex;
  };

  void DesktopEventQueueImpl::Init()
  {
    const int kQueueCapacity = 1024;
    mQueue.Init( kQueueCapacity );
  }

  void DesktopEventQueueImpl::QueuePush( DesktopEventType desktopEventType,
                                         void* dataBytes,
                                         int dataByteCount )
  {
    std::lock_guard< std::mutex > lockGuard( mMutex );

    //char buf[ 11 ] = {};
    //char clear[ 11 ] = {};
    //mQueue.QueuePush( "ass", 3 );
    //mQueue.QueuePush( "titty", 5 );

    //mQueue.QueuePop( buf, 3 );
    //MemCpy( buf, clear, 11 );

    //mQueue.QueuePush( "fuck", 4 );

    //mQueue.QueuePop( buf, 5 );
    //MemCpy( buf, clear, 11 );

    //mQueue.QueuePush( "balls", 4 );

    //mQueue.QueuePop( buf, 4 );
    //MemCpy( buf, clear, 11 );

    //mQueue.QueuePop( buf, 4 );
    //MemCpy( buf, clear, 11 );

    //static int asdf;
    //++asdf;

    mQueue.Push( &desktopEventType, sizeof( DesktopEventType ) );
    mQueue.Push( dataBytes, dataByteCount );
  }

  bool DesktopEventQueueImpl::Empty()
  {
    std::lock_guard< std::mutex > lockGuard( mMutex );
    return mQueue.Empty();

  }
  bool DesktopEventQueueImpl::QueuePop( void* dataBytes,
                                        int dataByteCount )
  {
    std::lock_guard< std::mutex > lockGuard( mMutex );
    return mQueue.Pop( dataBytes, dataByteCount );
  }

  static DesktopEventQueueImpl sEventQueue;


  // ---

  struct DesktopEventDataCreateWindow
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mWidth;
    int                 mHeight;
    int                 mX;
    int                 mY;
    void*               mNativeWindowHandle;
  };

  struct DesktopEventDataCursorUnobscured
  {
    DesktopWindowHandle mDesktopWindowHandle;
  };

  struct DesktopEventDataWindowResize
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mWidth;
    int                 mHeight;
  };

  struct DesktopEventDataKeyState
  {
    Key                 mKey;
    bool                mDown;
  };

  struct DesktopEventDataKeyInput
  {
    Codepoint mCodepoint;
  };

  struct DesktopEventDataMouseWheel
  {
    int mDelta;
  };

  struct DesktopEventDataMouseMove
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int mX;
    int mY;
  };

  struct DesktopEventDataWindowMove
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mX;
    int                 mY;
  };

  // ---


  DesktopEventQueue DesktopEventQueue::Instance;

  void DesktopEventQueue::Init()
  {
    sEventQueue.Init();
  }


  void DesktopEventQueue::ApplyQueuedEvents( DesktopWindowStateCollection* windowStates )
  {
    while( !sEventQueue.Empty() )
    {
      DesktopEventType desktopEventType = {};
      if( !sEventQueue.QueuePop( &desktopEventType, sizeof( DesktopEventType ) ) )
        break;

      switch( desktopEventType )
      {
        case DesktopEventType::WindowCreate:
        {
          DesktopEventDataCreateWindow data;
          sEventQueue.QueuePop( &data, sizeof( data ) );

          int iDesktopWindow = -1;
          for( int i = 0; i < kMaxDesktopWindowStateCount; ++i )
          {
            DesktopWindowState* desktopWindowState =
              DesktopWindowStateCollection::InstanceStuffThread.GetStateAtIndex( i );
            if( !IsWindowHandleValid( desktopWindowState->mDesktopWindowHandle ) )
            {
              iDesktopWindow = i;
            }
          }

          DesktopWindowState* desktopWindowState = windowStates->GetStateAtIndex( iDesktopWindow );
          desktopWindowState->mWidth = data.mWidth;
          desktopWindowState->mHeight = data.mHeight;
          desktopWindowState->mDesktopWindowHandle = data.mDesktopWindowHandle;
          desktopWindowState->mX = data.mX;
          desktopWindowState->mY = data.mY;
        } break;
        case DesktopEventType::CursorUnobscured:
        {
          DesktopEventDataCursorUnobscured data;
          sEventQueue.QueuePop( &data, sizeof( data ) );

          for( int iState = 0; iState < kMaxDesktopWindowStateCount; ++iState )
          {
            DesktopWindowState* state = windowStates->GetStateAtIndex( iState );
            const bool unobscured =
              state->mDesktopWindowHandle.mIndex ==
              data.mDesktopWindowHandle.mIndex;
            state->mCursorUnobscured = unobscured;
          }
        } break;
        case DesktopEventType::WindowMove:
        {
          DesktopEventDataWindowMove data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          DesktopWindowState* state = windowStates->FindDesktopWindowState( data.mDesktopWindowHandle );
          state->mX = data.mX;
          state->mY = data.mY;
        } break;
        case DesktopEventType::WindowResize:
        {
          DesktopEventDataWindowResize data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          DesktopWindowState* state = windowStates->FindDesktopWindowState( data.mDesktopWindowHandle );
          state->mWidth = data.mWidth;
          state->mHeight = data.mHeight;
        } break;
        case DesktopEventType::KeyInput:
        {
          DesktopEventDataKeyInput data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          KeyboardInput::Instance->mWMCharPressedHax = data.mCodepoint;
        } break;
        case DesktopEventType::KeyState:
        {
          DesktopEventDataKeyState data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          KeyboardInput::Instance->SetIsKeyDown( data.mKey, data.mDown );
        } break;
        case DesktopEventType::MouseWheel:
        {
          DesktopEventDataMouseWheel data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          KeyboardInput::Instance->mCurr.mMouseScroll += data.mDelta;
        } break;
        default:
        {
          OS::DebugBreak();
          TAC_ASSERT_MESSAGE( "..." );
          TAC_ASSERT( false );
          //TAC_INVALID_DEFAULT_CASE( desktopEventType );
        }
      }
    }
  }


  void DesktopEventQueue::PushEventCursorUnobscured( DesktopWindowHandle desktopWindowHandle )
  {
    DesktopEventDataCursorUnobscured data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    sEventQueue.QueuePush( DesktopEventType::CursorUnobscured, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventCreateWindow( DesktopWindowHandle desktopWindowHandle,
                                                 int width,
                                                 int height,
                                                 int x,
                                                 int y,
                                                 void* nativeWindowHandle )
  {
    DesktopEventDataCreateWindow data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mWidth = width;
    data.mHeight = height;
    data.mNativeWindowHandle = nativeWindowHandle;
    data.mX = x;
    data.mY = y;
    sEventQueue.QueuePush( DesktopEventType::WindowCreate, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventMoveWindow( DesktopWindowHandle desktopWindowHandle,
                                               int x,
                                               int y )
  {
    DesktopEventDataWindowMove data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mX = x;
    data.mY = y;
    sEventQueue.QueuePush( DesktopEventType::WindowMove, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventResizeWindow( DesktopWindowHandle desktopWindowHandle,
                                                 int w,
                                                 int h )
  {
    DesktopEventDataWindowResize data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mWidth = w;
    data.mHeight = h;
    sEventQueue.QueuePush( DesktopEventType::WindowResize, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventMouseWheel( int ticks )
  {
    DesktopEventDataMouseWheel data;
    data.mDelta = ticks;
    sEventQueue.QueuePush( DesktopEventType::MouseWheel, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventMouseMove( DesktopWindowHandle desktopWindowHandle,
                                              int x,
                                              int y )
  {
    DesktopEventDataMouseMove data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mX = x;
    data.mY = y;
    sEventQueue.QueuePush( DesktopEventType::MouseMove, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventKeyState( Key key, bool down )
  {
    DesktopEventDataKeyState data;
    data.mDown = down;
    data.mKey = key;
    sEventQueue.QueuePush( DesktopEventType::KeyState, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventKeyInput( Codepoint codepoint )
  {
    DesktopEventDataKeyInput data;
    data.mCodepoint = codepoint;
    sEventQueue.QueuePush( DesktopEventType::KeyInput, &data, sizeof( data ) );

  }

  thread_local ThreadType gThreadType = ThreadType::Unknown;

  //DesktopApp* DesktopApp::Instance = nullptr;

  //DesktopApp::DesktopApp()
  //{
  //  Instance = this;
  //}

  //DesktopApp::~DesktopApp()
  //{
  //}

  static ThreadAllocator sAllocatorStuff;
  static ThreadAllocator sAllocatorMain;

  static void DontMaxOutCpuGpuPowerUsage()
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }

  static void LogicThread()
  {
    Errors& errors = sLogicThreadErrors;
    TAC_ON_DESTRUCT( if( errors.size() ) OS::mShouldStopRunning = true );

    gThreadType = ThreadType::Stuff;

    sAllocatorStuff.Init( 1024 * 1024 * 10 );
    FrameMemory::SetThreadAllocator( &sAllocatorStuff );

    DesktopWindowManagerInit();


    TAC_NEW KeyboardInput;
    Shell::Instance.Init( errors );
    TAC_HANDLE_ERROR( errors );

    TAC_NEW Settings;
    Settings::Instance->Init( errors );
    Settings::Instance->Load( errors );

    TAC_NEW ProfileSystem;
    ProfileSystem::Instance->Init();

    Render::Init();

    TAC_NEW FontStuff;
    FontStuff::Instance->Load( errors );
    TAC_HANDLE_ERROR( errors );

    sAppInterfaceProject.mProjectInit( errors );
    TAC_HANDLE_ERROR( errors );

    while( !OS::mShouldStopRunning )
    {
      KeyboardInput::Instance->BeginFrame();

      if( ControllerInput::Instance )
        ControllerInput::Instance->Update();

      Shell::Instance.Update( errors );
      TAC_HANDLE_ERROR( errors );

      sAppInterfaceProject.mProjectUpdate( errors );
      TAC_HANDLE_ERROR( errors );

      KeyboardInput::Instance->EndFrame();

      Render::SubmitFrame();

      DontMaxOutCpuGpuPowerUsage();
    }
  }

  static void PlatformThread()
  {
    Errors& errors = sPlatformThreadErrors;
    TAC_ON_DESTRUCT( if( errors.size() ) OS::mShouldStopRunning = true );
    while( !OS::mShouldStopRunning )
    {
      sAppInterfacePlatform.mPlatformPoll( errors );
      TAC_HANDLE_ERROR( errors );

      DesktopWindowPollCreationRequests( errors );
      TAC_HANDLE_ERROR( errors );

      Render::RenderFrame( errors );
      TAC_HANDLE_ERROR( errors );

      DontMaxOutCpuGpuPowerUsage();
    }
  }

  void DesktopAppInit( AppInterfacePlatform appInterfacePlatform, Errors& errors )
  {
    gThreadType = ThreadType::Main;
    sAllocatorMain.Init( 1024 * 1024 * 10 );
    FrameMemory::SetThreadAllocator( &sAllocatorMain );
    sEventQueue.Init();

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

    sAppInterfacePlatform = appInterfacePlatform;
    sAppInterfaceProject.mProjectInit = info.mProjectInit;
    sAppInterfaceProject.mProjectUpdate = info.mProjectUpdate;

    // right place?
    Shell::Instance.mAppName = appName;
    Shell::Instance.mPrefPath = prefPath;
    Shell::Instance.mInitialWorkingDir = workingDir;


    TAC_HANDLE_ERROR( errors );

  }


  void DesktopAppRun( Errors& errors )
  {
    TAC_HANDLE_ERROR( errors );

    std::thread threads[] =
    {
      std::thread( LogicThread ),
    };

    PlatformThread();

    for( std::thread& thread : threads )
      thread.join();
  }

}
