#include "src/common/containers/tacFrameVector.h"
#include "src/common/containers/tacRingBuffer.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI.h"
//#include "src/common/
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
#include "src/common/tacIDCollection.h"
#include "src/common/containers/tacFixedVector.h"

#include <mutex>

namespace Tac
{
  enum class DesktopEventType
  {
    Unknown = 0,
    WindowAssignHandle,
    WindowResize,
    WindowMove,
    CursorUnobscured,
    KeyState,
    KeyInput,
    MouseWheel,
    MouseMove,
  };
  struct WantSpawnInfo
  {
    DesktopWindowHandle mHandle;
    int mX;
    int mY;
    int mWidth;
    int mHeight;
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

  typedef FixedVector< WantSpawnInfo, kMaxDesktopWindowStateCount > WindowRequests;

  Errors                       gPlatformThreadErrors;
  Errors                       gLogicThreadErrors;
  AppInterfacePlatform         sAppInterfacePlatform;
  AppInterfaceProject          sAppInterfaceProject;
  static std::mutex            sWindowHandleLock;
  static IdCollection          sDesktopWindowHandleIDs;
  static WindowRequests        sWindowRequests;
  thread_local ThreadType      gThreadType = ThreadType::Unknown;
  static ThreadAllocator       sAllocatorStuff;
  static ThreadAllocator       sAllocatorMain;
  static DesktopEventQueueImpl sEventQueue;

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


  // ---

  //struct DesktopEventDataCreateWindow
  //{
  //  DesktopWindowHandle mDesktopWindowHandle;
  //  int                 mWidth;
  //  int                 mHeight;
  //  int                 mX;
  //  int                 mY;
  //  void*               mNativeWindowHandle;
  //};

  struct DesktopEventDataAssignHandle
  {
    DesktopWindowHandle mDesktopWindowHandle;
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


  void DesktopEventQueue::ApplyQueuedEvents( DesktopWindowStates* desktopWindowStates )
  {
    while( !sEventQueue.Empty() )
    {
      DesktopEventType desktopEventType = {};
      if( !sEventQueue.QueuePop( &desktopEventType, sizeof( DesktopEventType ) ) )
        break;

      switch( desktopEventType )
      {
        case DesktopEventType::WindowAssignHandle:
        {
          DesktopEventDataAssignHandle data;
          sEventQueue.QueuePop( &data, sizeof( data ) );

          DesktopWindowState* desktopWindowState = &( *desktopWindowStates )[ data.mDesktopWindowHandle.mIndex ];
          desktopWindowState->mNativeWindowHandle = data.mNativeWindowHandle;
        } break;

        //case DesktopEventType::WindowCreate:
        //{
        //  DesktopEventDataCreateWindow data;
        //  sEventQueue.QueuePop( &data, sizeof( data ) );

        //  int iDesktopWindow = -1;
        //  for( int i = 0; i < kMaxDesktopWindowStateCount; ++i )
        //  {
        //    DesktopWindowState* desktopWindowState =
        //      DesktopWindowStateCollection::InstanceStuffThread.GetStateAtIndex( i );
        //    if( !IsWindowHandleValid( desktopWindowState->mDesktopWindowHandle ) )
        //    {
        //      iDesktopWindow = i;
        //    }
        //  }

        //  DesktopWindowState* desktopWindowState = windowStates->GetStateAtIndex( iDesktopWindow );
        //  desktopWindowState->mWidth = data.mWidth;
        //  desktopWindowState->mHeight = data.mHeight;
        //  desktopWindowState->mDesktopWindowHandle = data.mDesktopWindowHandle;
        //  desktopWindowState->mNativeWindowHandle;
        //  desktopWindowState->mX = data.mX;
        //  desktopWindowState->mY = data.mY;
        //} break;
        case DesktopEventType::CursorUnobscured:
        {
          DesktopEventDataCursorUnobscured data;
          sEventQueue.QueuePop( &data, sizeof( data ) );

          for( int iState = 0; iState < kMaxDesktopWindowStateCount; ++iState )
          {
            DesktopWindowState* state = &( *desktopWindowStates )[ iState ];
            const bool unobscured = iState == data.mDesktopWindowHandle.mIndex;
            state->mCursorUnobscured = unobscured;
          }
        } break;
        case DesktopEventType::WindowMove:
        {
          DesktopEventDataWindowMove data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          DesktopWindowState* state = &( *desktopWindowStates )[ data.mDesktopWindowHandle.mIndex ];
          state->mX = data.mX;
          state->mY = data.mY;
        } break;
        case DesktopEventType::WindowResize:
        {
          DesktopEventDataWindowResize data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          DesktopWindowState* state = &( *desktopWindowStates )[ data.mDesktopWindowHandle.mIndex ];
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
        case DesktopEventType::MouseMove:
        {
          DesktopEventDataMouseMove data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          KeyboardInput::Instance->mCurr.mScreenspaceCursorPos =
          {
            ( float )data.mX,
            ( float )data.mY
          };
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

  //void DesktopEventQueue::PushEventCreateWindow( DesktopWindowHandle desktopWindowHandle,
  //                                               int width,
  //                                               int height,
  //                                               int x,
  //                                               int y,
  //                                               void* nativeWindowHandle )
  //{
  //  DesktopEventDataCreateWindow data;
  //  data.mDesktopWindowHandle = desktopWindowHandle;
  //  data.mWidth = width;
  //  data.mHeight = height;
  //  data.mNativeWindowHandle = nativeWindowHandle;
  //  data.mX = x;
  //  data.mY = y;
  //  sEventQueue.QueuePush( DesktopEventType::WindowCreate, &data, sizeof( data ) );
  //}
  void DesktopEventQueue::PushEventAssignHandle( DesktopWindowHandle desktopWindowHandle,
                                                 void* nativeWindowHandle )
  {

    DesktopEventDataAssignHandle data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mNativeWindowHandle = nativeWindowHandle;
    sEventQueue.QueuePush( DesktopEventType::WindowAssignHandle, &data, sizeof( data ) );
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


  //DesktopApp* DesktopApp::Instance = nullptr;

  //DesktopApp::DesktopApp()
  //{
  //  Instance = this;
  //}

  //DesktopApp::~DesktopApp()
  //{
  //}


  static void DontMaxOutCpuGpuPowerUsage()
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }

  static void LogicThread()
  {
    Errors& errors = gLogicThreadErrors;
    TAC_ON_DESTRUCT( if( errors.size() ) OS::mShouldStopRunning = true );

    gThreadType = ThreadType::Stuff;

    sAllocatorStuff.Init( 1024 * 1024 * 10 );
    FrameMemory::SetThreadAllocator( &sAllocatorStuff );

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
    Errors& errors = gPlatformThreadErrors;
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
    sDesktopWindowHandleIDs.Init( kMaxDesktopWindowStateCount );
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
  //static DesktopWindow* mDesktopWindows[ kMaxDesktopWindowStateCount ] = {};

  WindowHandleIterator::WindowHandleIterator()
  {
    sWindowHandleLock.lock();
  }

  WindowHandleIterator::~WindowHandleIterator()
  {
    sWindowHandleLock.unlock();

  }

  int* WindowHandleIterator::begin()
  {
    return sDesktopWindowHandleIDs.begin();
  }

  int* WindowHandleIterator::end()
  {
    return sDesktopWindowHandleIDs.end();
  }

  //  WindowHandleIterator GetDesktopWindowHandleIDs()
  //  {
  //WindowHandleIterator
  //  }

  //DesktopWindowHandle* GetDesktopWindowHandles()
  //{
  //  sWindowHandleLock.lock();
  //  DesktopWindowHandle* result = FrameMemory::Allocate(
  //    sizeof( DesktopWindowHandle ) *
  //    sDesktopWindowHandleIDs. );

  //    void* Allocate( int );
  //  sWindowHandleLock.unlock();
  //}

  //void DesktopWindowManager::SetWindowParams( WindowParams windowParams )
  //{
  //  WindowParams* pParams = FindWindowParams( windowParams.mName );
  //  if( pParams )
  //    *pParams = windowParams;
  //  else
  //    mWindowParams.push_back( windowParams );
  //}

  //WindowParams* DesktopWindowManager::FindWindowParams( StringView windowName )
  //{
  //  for( WindowParams& data : mWindowParams )
  //    if( data.mName == windowName )
  //      return &data;
  //  return nullptr;
  //}

  //void DesktopWindowManager::DoWindow( StringView windowName )
  //{
  //  DesktopWindow* window = DesktopApp::Instance->FindWindow( windowName );
  //  if( !window )
  //    mWantSpawnWindows.push_back( windowName );
  //}

  void DesktopWindowPollCreationRequests( Errors& errors )
  {
    WindowRequests requests;
    sWindowHandleLock.lock();
    requests = sWindowRequests;
    sWindowHandleLock.unlock();
    for( WantSpawnInfo info : requests )
      sAppInterfacePlatform.mPlatformSpawnWindow( info.mHandle,
                                                  info.mX,
                                                  info.mY,
                                                  info.mWidth,
                                                  info.mHeight );
    sWindowRequests.clear();
  }

  DesktopWindowHandle DesktopWindowCreate( int x, int y, int width, int height )
  {
    std::lock_guard< std::mutex > lock( sWindowHandleLock );
    const DesktopWindowHandle handle = { sDesktopWindowHandleIDs.Alloc() };
    WantSpawnInfo info;
    info.mX = x;
    info.mY = y;
    info.mWidth = width;
    info.mHeight = height;
    info.mHandle = handle;
    sWindowRequests.push_back( info );
    return handle;
  }

}
