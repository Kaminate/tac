#include "src/common/containers/tacFixedVector.h"
#include "src/common/containers/tacFrameVector.h"
#include "src/common/containers/tacRingBuffer.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacControllerInput.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacIDCollection.h"
#include "src/common/tacKeyboardInput.h"
#include "src/common/tacOS.h"
#include "src/common/tacSettings.h"
#include "src/common/tacString.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowGraphics.h"

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
    int                 mX;
    int                 mY;
    int                 mWidth;
    int                 mHeight;
  };

  struct DesktopEventQueueImpl
  {
    void       Init();
    void       QueuePush( DesktopEventType, void*, int );
    bool       QueuePop( void*, int );
    bool       Empty();
    RingBuffer mQueue;
    std::mutex mMutex;
  };

  typedef FixedVector< WantSpawnInfo, kDesktopWindowCapacity > WindowRequests;

  Errors                       gPlatformThreadErrors( Errors::Flags::kDebugBreakOnAppend );
  Errors                       gLogicThreadErrors( Errors::Flags::kDebugBreakOnAppend );
  static AppInterfacePlatform  sAppInterfacePlatform;
  static AppInterfaceProject   sAppInterfaceProject;
  static std::mutex            sWindowHandleLock;
  static IdCollection          sDesktopWindowHandleIDs( kDesktopWindowCapacity );
  static WindowRequests        sWindowRequests;
  static ThreadAllocator       sAllocatorStuff;
  static ThreadAllocator       sAllocatorMain;
  static DesktopEventQueueImpl sEventQueue;
  thread_local ThreadType      gThreadType = ThreadType::Unknown;

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

  struct DesktopEventDataAssignHandle
  {
    DesktopWindowHandle mDesktopWindowHandle;
    const void*         mNativeWindowHandle;
    int                 mX;
    int                 mY;
    int                 mW;
    int                 mH;
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
    // Position of the mouse relative to the top left corner of the window
    int                 mX;
    int                 mY;
  };

  struct DesktopEventDataWindowMove
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mX;
    int                 mY;
  };

  void DesktopEventInit()
  {
    sEventQueue.Init();
  }

  void DesktopEventApplyQueue( DesktopWindowState* desktopWindowStates )
  {
    TAC_ASSERT( gThreadType == ThreadType::Stuff );
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
          DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
          if( !desktopWindowState->mNativeWindowHandle )
          {
            WindowGraphicsNativeHandleChanged( data.mDesktopWindowHandle,
                                               data.mNativeWindowHandle,
                                               data.mX,
                                               data.mY,
                                               data.mW,
                                               data.mH );
          }
          desktopWindowState->mNativeWindowHandle = data.mNativeWindowHandle;
          desktopWindowState->mWidth = data.mW;
          desktopWindowState->mHeight = data.mH;
          desktopWindowState->mX = data.mX;
          desktopWindowState->mY = data.mY;
        } break;

        case DesktopEventType::WindowMove:
        {
          DesktopEventDataWindowMove data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          DesktopWindowState* state = GetDesktopWindowState( data.mDesktopWindowHandle );
          state->mX = data.mX;
          state->mY = data.mY;
        } break;

        case DesktopEventType::WindowResize:
        {
          DesktopEventDataWindowResize data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
          desktopWindowState->mWidth = data.mWidth;
          desktopWindowState->mHeight = data.mHeight;
          WindowGraphicsResize( data.mDesktopWindowHandle,
                                desktopWindowState->mWidth,
                                desktopWindowState->mHeight );
        } break;

        case DesktopEventType::KeyInput:
        {
          DesktopEventDataKeyInput data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          gKeyboardInput.mWMCharPressedHax = data.mCodepoint;
        } break;

        case DesktopEventType::KeyState:
        {
          DesktopEventDataKeyState data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          gKeyboardInput.SetIsKeyDown( data.mKey, data.mDown );
        } break;

        case DesktopEventType::MouseWheel:
        {
          DesktopEventDataMouseWheel data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          gKeyboardInput.mCurr.mMouseScroll += data.mDelta;
        } break;

        case DesktopEventType::MouseMove:
        {
          DesktopEventDataMouseMove data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          // ...
        } break;

        case DesktopEventType::CursorUnobscured:
        {
          DesktopEventDataCursorUnobscured data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          SetHoveredWindow( data.mDesktopWindowHandle );
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

  void DesktopEventAssignHandle( const DesktopWindowHandle desktopWindowHandle,
                                 const void* nativeWindowHandle,
                                 const int x,
                                 const int y,
                                 const int w,
                                 const int h )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataAssignHandle data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mNativeWindowHandle = nativeWindowHandle;
    data.mW = w;
    data.mH = h;
    data.mX = x;
    data.mY = y;
    sEventQueue.QueuePush( DesktopEventType::WindowAssignHandle, &data, sizeof( data ) );
  }

  void DesktopEventMoveWindow( const DesktopWindowHandle desktopWindowHandle,
                               const int x,
                               const int y )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataWindowMove data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mX = x;
    data.mY = y;
    sEventQueue.QueuePush( DesktopEventType::WindowMove, &data, sizeof( data ) );
  }

  void DesktopEventResizeWindow( const DesktopWindowHandle desktopWindowHandle,
                                 const int w,
                                 const int h )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataWindowResize data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mWidth = w;
    data.mHeight = h;
    sEventQueue.QueuePush( DesktopEventType::WindowResize, &data, sizeof( data ) );
  }

  void DesktopEventMouseWheel( const int ticks )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataMouseWheel data;
    data.mDelta = ticks;
    sEventQueue.QueuePush( DesktopEventType::MouseWheel, &data, sizeof( data ) );
  }

  void DesktopEventMouseMove( const DesktopWindowHandle desktopWindowHandle,
                              const int x,
                              const int y )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataMouseMove data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    data.mX = x;
    data.mY = y;
    sEventQueue.QueuePush( DesktopEventType::MouseMove, &data, sizeof( data ) );
  }

  void DesktopEventKeyState( const Key key, const bool down )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataKeyState data;
    data.mDown = down;
    data.mKey = key;
    sEventQueue.QueuePush( DesktopEventType::KeyState, &data, sizeof( data ) );
  }

  void DesktopEventKeyInput( const Codepoint codepoint )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataKeyInput data;
    data.mCodepoint = codepoint;
    sEventQueue.QueuePush( DesktopEventType::KeyInput, &data, sizeof( data ) );
  }

  void DesktopEventMouseHoveredWindow( const DesktopWindowHandle desktopWindowHandle )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataCursorUnobscured data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    sEventQueue.QueuePush( DesktopEventType::CursorUnobscured, &data, sizeof( data ) );
  }

  static void DontMaxOutCpuGpuPowerUsage()
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }

  static void LogicThreadInit( Errors& errors )
  {
    gThreadType = ThreadType::Stuff;
    sAllocatorStuff.Init( 1024 * 1024 * 10 );
    FrameMemory::SetThreadAllocator( &sAllocatorStuff );

    Shell::Instance.Init( errors );
    TAC_HANDLE_ERROR( errors );

		SettingsInit( errors );
    TAC_HANDLE_ERROR( errors );

    TAC_NEW ProfileSystem;
    ProfileSystem::Instance->Init();

    TAC_NEW FontStuff;
    gFontStuff.Load( errors );
    TAC_HANDLE_ERROR( errors );

    sAppInterfaceProject.mProjectInit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  static void LogicThread()
  {
    Errors& errors = gLogicThreadErrors;
    TAC_ON_DESTRUCT( if( errors.size() ) OS::mShouldStopRunning = true );
    LogicThreadInit( errors );
    TAC_HANDLE_ERROR( errors );

    while( !OS::mShouldStopRunning )
    {
      gKeyboardInput.BeginFrame();

      ImGuiFrameBegin( Shell::Instance.mElapsedSeconds,
                       sAppInterfacePlatform.mPlatformGetMouseHoveredWindow() );

      if( ControllerInput::Instance )
        ControllerInput::Instance->Update();

      Shell::Instance.Update( errors );
      TAC_HANDLE_ERROR( errors );


			DesktopEventApplyQueue( GetDesktopWindowState( 0 ) );
      sAppInterfaceProject.mProjectUpdate( errors );
      TAC_HANDLE_ERROR( errors );

      ImGuiFrameEnd( errors );
      TAC_HANDLE_ERROR( errors );

      gKeyboardInput.EndFrame();

      Render::SubmitFrame();

      DontMaxOutCpuGpuPowerUsage();
    }
  }

  static void PlatformThreadInit( Errors& errors )
  {
    gThreadType = ThreadType::Main;
    sAllocatorMain.Init( 1024 * 1024 * 10 );
    FrameMemory::SetThreadAllocator( &sAllocatorMain );
  }

  static void PlatformThread()
  {
    Errors& errors = gPlatformThreadErrors;
    TAC_ON_DESTRUCT( if( errors.size() ) OS::mShouldStopRunning = true );
    PlatformThreadInit( errors );
    TAC_HANDLE_ERROR( errors );

    while( !OS::mShouldStopRunning )
    {
      sAppInterfacePlatform.mPlatformPoll( errors );
      TAC_HANDLE_ERROR( errors );

      DesktopAppUpdate( errors );
      TAC_HANDLE_ERROR( errors );

      Render::RenderFrame( errors );
      TAC_HANDLE_ERROR( errors );

      DontMaxOutCpuGpuPowerUsage();
    }
  }

  void DesktopAppInit( AppInterfacePlatform appInterfacePlatform, Errors& errors )
  {
    sEventQueue.Init();

    ExecutableStartupInfo info;
    info.Init( errors );
    TAC_HANDLE_ERROR( errors );

    String appDataPath;
    bool appDataPathExists;
    OS::GetApplicationDataPath( appDataPath, errors );
    OS::DoesFolderExist( appDataPath, appDataPathExists, errors );
    TAC_HANDLE_ERROR( errors );
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

    Render::Init();
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

  void DesktopAppUpdate( Errors& errors )
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

  DesktopWindowHandle DesktopAppCreateWindow( int x, int y, int width, int height )
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
