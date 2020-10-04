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

  enum class DesktopEventType
  {
    Unknown,
    CreateWindow,
    ResizeWindow,
    MoveWindow,
    CursorUnobscured,
    KeyState,
    KeyInput,
    MouseWheel,
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
        case DesktopEventType::CreateWindow:
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
        case DesktopEventType::MoveWindow:
        {
          DesktopEventDataWindowMove data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          DesktopWindowState* state = windowStates->FindDesktopWindowState( data.mDesktopWindowHandle );
          state->mX = data.mX;
          state->mY = data.mY;
        } break;
        case DesktopEventType::ResizeWindow:
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
    sEventQueue.QueuePush( DesktopEventType::CreateWindow, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventMoveWindow( DesktopWindowHandle desktopWindowHandle,
                            int x,
                            int y )
  {
    DesktopEventDataWindowMove data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mX = x;
    data.mY = y;
    sEventQueue.QueuePush( DesktopEventType::MoveWindow, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventResizeWindow( DesktopWindowHandle desktopWindowHandle,
                              int w,
                              int h )
  {
    DesktopEventDataWindowResize data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mWidth = w;
    data.mHeight = h;
    sEventQueue.QueuePush( DesktopEventType::ResizeWindow, &data, sizeof( data ) );
  }

  void DesktopEventQueue::PushEventMouseWheel( int ticks )
  {
    DesktopEventDataMouseWheel data;
    data.mDelta = ticks;
    sEventQueue.QueuePush( DesktopEventType::MouseWheel, &data, sizeof( data ) );
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

  DesktopApp* DesktopApp::Instance = nullptr;

  DesktopApp::DesktopApp()
  {
    Instance = this;
  }

  DesktopApp::~DesktopApp()
  {
  }

  static ThreadAllocator sAllocatorStuff;
  static ThreadAllocator sAllocatorMain;

  static void DontMaxOutCpuGpuPowerUsage()
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }

  //static void StuffThread( void* userData )
  static void StuffThread()
  {
    Errors& errors = DesktopApp::Instance->mErrorsStuffThread;
    TAC_ON_DESTRUCT( if( errors.size() ) OS::mShouldStopRunning = true );

    gThreadType = ThreadType::Stuff;

    sAllocatorStuff.Init( 1024 * 1024 * 10 );
    FrameMemory::SetThreadAllocator( &sAllocatorStuff );

    
    TAC_NEW KeyboardInput;
    TAC_NEW Shell;
    Shell::Instance->mAppName = DesktopApp::Instance->mAppName;
    Shell::Instance->mPrefPath = DesktopApp::Instance->mPrefPath;
    Shell::Instance->mInitialWorkingDir = DesktopApp::Instance->mInitialWorkingDir;
    Shell::Instance->Init( errors );
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

    DesktopApp::Instance->CreateControllerInput( errors );
    TAC_HANDLE_ERROR( errors );

    DesktopApp::Instance->mProjectInit( errors );

    while( !OS::mShouldStopRunning )
    {
      KeyboardInput::Instance->BeginFrame();
      Shell::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );

      DesktopApp::Instance->mProjectUpdate( errors );
      TAC_HANDLE_ERROR( errors );
      KeyboardInput::Instance->EndFrame();

      Render::SubmitFrame();

      DontMaxOutCpuGpuPowerUsage();
    }
  }

  static void MainThread()
  {
    //TAC_NEW KeyboardInput;
    Errors& errors = DesktopApp::Instance->mErrorsMainThread;
    TAC_ON_DESTRUCT( if( errors.size() ) OS::mShouldStopRunning = true );
    while( !OS::mShouldStopRunning )
    {
      DesktopApp::Instance->Poll( errors );
      TAC_HANDLE_ERROR( errors );

      DesktopWindowManager::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );

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
    sEventQueue.Init();

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

    mAppName = appName;
    mPrefPath = prefPath;
    mInitialWorkingDir = workingDir;
    mProjectInit = info.mProjectInit;
    mProjectUpdate = info.mProjectUpdate;


    TAC_HANDLE_ERROR( errors );

  }

  DesktopWindow* DesktopApp::FindDesktopWindow( DesktopWindowHandle desktopWindowHandle )
  {
    for( int i = 0; i < kMaxDesktopWindowStateCount; ++i )
    {
      DesktopWindow* desktopWindow = mDesktopWindows[ i ];
      if( !desktopWindow )
        continue;
      if( AreWindowHandlesEqual( desktopWindow->mHandle, desktopWindowHandle ) )
        return desktopWindow;
    }
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

}
