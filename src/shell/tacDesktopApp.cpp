#include "src/common/containers/tacFixedVector.h"
#include "src/common/containers/tacFrameVector.h"
#include "src/common/containers/tacRingBuffer.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/shell/tacShell.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/string/tacString.h"
#include "src/common/tacControllerInput.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacIDCollection.h"
#include "src/common/tacKeyboardInput.h"
#include "src/common/tacNet.h"
#include "src/common/tacOS.h"
#include "src/common/tacSettings.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowGraphics.h"

#include <mutex>

namespace Tac
{
  enum class ThreadType
  {
    Unknown,
    Main,
    Stuff
  };
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
    int                 mX = 0;
    int                 mY = 0;
    int                 mWidth = 0;
    int                 mHeight = 0;
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

  struct RequestMove
  {
    bool              mRequested = false;
    DesktopWindowRect mRect = {};
  };

  struct RequestResize
  {
    bool              mRequested = false;
    int               mEdgePx = 0;
  };

  typedef FixedVector< WantSpawnInfo, kDesktopWindowCapacity > WindowRequestsCreate;
  typedef FixedVector< DesktopWindowHandle, kDesktopWindowCapacity > WindowRequestsDestroy;

  static Errors                        gPlatformThreadErrors( Errors::Flags::kDebugBreakOnAppend );
  static Errors                        gLogicThreadErrors( Errors::Flags::kDebugBreakOnAppend );
  static PlatformSpawnWindow           sPlatformSpawnWindow;
  static PlatformDespawnWindow         sPlatformDespawnWindow;
  static PlatformGetMouseHoveredWindow sPlatformGetMouseHoveredWindow;
  static PlatformFrameEnd              sPlatformFrameEnd;
  static PlatformFrameBegin            sPlatformFrameBegin;
  static PlatformWindowMoveControls    sPlatformWindowMoveControls;
  static PlatformWindowResizeControls  sPlatformWindowResizeControls;
  static ProjectInit                   sProjectInit;
  static ProjectUpdate                 sProjectUpdate;
  static ProjectUninit                 sProjectUninit;
  static std::mutex                    sWindowHandleLock;
  static IdCollection                  sDesktopWindowHandleIDs( kDesktopWindowCapacity );
  static WindowRequestsCreate          sWindowRequestsCreate;
  static WindowRequestsDestroy         sWindowRequestsDestroy;
  static ThreadAllocator               sAllocatorStuff;
  static ThreadAllocator               sAllocatorMain;
  static DesktopEventQueueImpl         sEventQueue;
  static RequestMove                   sRequestMove[ kDesktopWindowCapacity ];
  static RequestResize                 sRequestResize[ kDesktopWindowCapacity ];
  thread_local ThreadType              gThreadType = ThreadType::Unknown;


  static void DesktopAppUpdateWindowRequests()
  {
    WindowRequestsCreate requestsCreate;
    WindowRequestsDestroy requestsDestroy;
    {
      sWindowHandleLock.lock();
      if( sWindowRequestsCreate.size() )
      {
        requestsCreate = sWindowRequestsCreate;
        sWindowRequestsCreate.clear();
      }
      if( sWindowRequestsDestroy.size() )
      {
        requestsDestroy = sWindowRequestsDestroy;
        sWindowRequestsDestroy.clear();
      }
      sWindowHandleLock.unlock();
    }

    for( const WantSpawnInfo& info : requestsCreate )
      sPlatformSpawnWindow( info.mHandle,
                            info.mX,
                            info.mY,
                            info.mWidth,
                            info.mHeight );
    for( DesktopWindowHandle desktopWindowHandle : requestsDestroy )
      sPlatformDespawnWindow( desktopWindowHandle );
  }

  static void DesktopAppUpdateMoveResize()
  {
    for( int i = 0; i < kDesktopWindowCapacity; ++i )
    {
      const DesktopWindowHandle desktopWindowHandle = { i };
      const DesktopWindowState* desktopWindowState = GetDesktopWindowState( desktopWindowHandle );
      if( !desktopWindowState->mNativeWindowHandle )
        continue;

      const RequestMove* requestMove = &sRequestMove[ i ];
      if( requestMove->mRequested )
      {
        const bool useRect = requestMove->mRect.mLeft != 0 && requestMove->mRect.mRight != 0;
        const DesktopWindowRect desktopWindowRect = requestMove->mRect.IsEmpty()
          ? GetDesktopWindowRectWindowspace( desktopWindowHandle )
          : requestMove->mRect;
        sPlatformWindowMoveControls( desktopWindowHandle, desktopWindowRect );
        sRequestMove[ i ] = RequestMove();
      }

      const RequestResize* requestResize = &sRequestResize[ i ];
      if( requestResize->mRequested )
      {
        sPlatformWindowResizeControls( desktopWindowHandle, requestResize->mEdgePx );
        sRequestResize[ i ] = RequestResize();
      }
    }
  }

  static void DontMaxOutCpuGpuPowerUsage()
  {
    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }

  static void LogicThreadInit( Errors& errors )
  {
    gThreadType = ThreadType::Stuff;
    sAllocatorStuff.Init( 1024 * 1024 * 10 );
    FrameMemorySetThreadAllocator( &sAllocatorStuff );

    ShellInit( errors );
    TAC_HANDLE_ERROR( errors );

    SettingsInit( errors );
    TAC_HANDLE_ERROR( errors );

    TAC_NEW FontStuff;
    gFontStuff.Load( errors );
    TAC_HANDLE_ERROR( errors );

    ImGuiInit();

    sProjectInit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  static void LogicThreadUninit()
  {
    ImGuiUninit();
  }

  static void LogicThread()
  {
    Errors& errors = gLogicThreadErrors;
    TAC_ON_DESTRUCT(
      {
        LogicThreadUninit();
        if( errors.size() )
          OSAppStopRunning();
        Render::SubmitFinish();
      } );
    LogicThreadInit( errors );
    TAC_HANDLE_ERROR( errors );

    while( OSAppIsRunning() )
    {
      TAC_PROFILE_BLOCK;
      ProfileSetGameFrame();

      if( Net::Instance )
      {
        Net::Instance->Update( errors );
        TAC_HANDLE_ERROR( errors );
      }

      {
        TAC_PROFILE_BLOCK_NAMED( "update timer" );

        ShellTimerUpdate();
        while( ShellTimerFrame() )
        {
        }
      }

      {
        DesktopEventApplyQueue();

        // To reduce input latency, update the game soon after polling the controller.

        gKeyboardInput.BeginFrame();
        ImGuiFrameBegin( ShellGetElapsedSeconds(),
                         sPlatformGetMouseHoveredWindow() );

        if( ControllerInput::Instance )
          ControllerInput::Instance->Update();

        sProjectUpdate( errors );
        TAC_HANDLE_ERROR( errors );

        ImGuiFrameEnd( errors );
        TAC_HANDLE_ERROR( errors );

        gKeyboardInput.EndFrame();
      }

      Render::SubmitFrame();

      DontMaxOutCpuGpuPowerUsage();
    }

  }

  static void PlatformThreadUninit()
  {

  }

  static void PlatformThreadInit( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    gThreadType = ThreadType::Main;
    sAllocatorMain.Init( 1024 * 1024 * 10 );
    FrameMemorySetThreadAllocator( &sAllocatorMain );
  }

  static void PlatformThread()
  {
    Errors& errors = gPlatformThreadErrors;
    TAC_ON_DESTRUCT(
      {
        PlatformThreadUninit();
        if( errors.size() )
          OSAppStopRunning();
        Render::RenderFinish();
      } );
    PlatformThreadInit( errors );
    TAC_HANDLE_ERROR( errors );

    while( OSAppIsRunning() )
    {
      TAC_PROFILE_BLOCK;

      sPlatformFrameBegin( errors );
      TAC_HANDLE_ERROR( errors );

      DesktopAppUpdate( errors );
      TAC_HANDLE_ERROR( errors );

      sPlatformFrameEnd( errors );
      TAC_HANDLE_ERROR( errors );

      Render::RenderFrame( errors );
      TAC_HANDLE_ERROR( errors );

      DontMaxOutCpuGpuPowerUsage();
    }

  }

  //WindowHandleIterator::WindowHandleIterator() {  }
  //WindowHandleIterator::~WindowHandleIterator() {  }

  int* WindowHandleIterator::begin()
  {
    TAC_ASSERT( IsLogicThread() );
    sWindowHandleLock.lock();
    return sDesktopWindowHandleIDs.begin();
  }

  int* WindowHandleIterator::end()
  {
    int* result = sDesktopWindowHandleIDs.end();
    sWindowHandleLock.unlock();
    return result;
  }

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
    // Tac::WindowProc still spews out events while a popupbox is open
    if( mQueue.capacity() - mQueue.size() < sizeof( DesktopEventType ) + dataByteCount )
      return;
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
    const void*         mNativeWindowHandle = nullptr;
    int                 mX = 0;
    int                 mY = 0;
    int                 mW = 0;
    int                 mH = 0;
  };

  struct DesktopEventDataCursorUnobscured
  {
    DesktopWindowHandle mDesktopWindowHandle;
  };

  struct DesktopEventDataWindowResize
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mWidth = 0;
    int                 mHeight = 0;
  };

  struct DesktopEventDataKeyState
  {
    Key                 mKey = Key::Count;
    bool                mDown = false;
  };

  struct DesktopEventDataKeyInput
  {
    Codepoint mCodepoint = 0;
  };

  struct DesktopEventDataMouseWheel
  {
    int mDelta = 0;
  };

  struct DesktopEventDataMouseMove
  {
    DesktopWindowHandle mDesktopWindowHandle;
    // Position of the mouse relative to the top left corner of the window
    int                 mX = 0;
    int                 mY = 0;
  };

  struct DesktopEventDataWindowMove
  {
    DesktopWindowHandle mDesktopWindowHandle;
    int                 mX = 0;
    int                 mY = 0;
  };

  void                DesktopEventInit()
  {
    sEventQueue.Init();
  }

  void                DesktopEventApplyQueue()// DesktopWindowState* desktopWindowStates )
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
          if( desktopWindowState->mNativeWindowHandle != data.mNativeWindowHandle )
            //if( !desktopWindowState->mNativeWindowHandle )
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
          gKeyboardInput.mMouseDeltaScroll =
            gKeyboardInput.mCurr.mMouseScroll -
            gKeyboardInput.mPrev.mMouseScroll;
        } break;

        case DesktopEventType::MouseMove:
        {
          DesktopEventDataMouseMove data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          const DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
          gKeyboardInput.mCurr.mScreenspaceCursorPos = {
            ( float )desktopWindowState->mX + ( float )data.mX,
            ( float )desktopWindowState->mY + ( float )data.mY };
          gKeyboardInput.mMouseDeltaPos =
            gKeyboardInput.mCurr.mScreenspaceCursorPos -
            gKeyboardInput.mPrev.mScreenspaceCursorPos;
        } break;

        case DesktopEventType::CursorUnobscured:
        {
          DesktopEventDataCursorUnobscured data;
          sEventQueue.QueuePop( &data, sizeof( data ) );
          SetHoveredWindow( data.mDesktopWindowHandle );
        } break;

        default:
          TAC_CRITICAL_ERROR_INVALID_CASE( desktopEventType );
          break;
      }
    }
  }

  void                DesktopEventAssignHandle( const DesktopWindowHandle desktopWindowHandle,
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

  void                DesktopEventMoveWindow( const DesktopWindowHandle desktopWindowHandle,
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

  void                DesktopEventResizeWindow( const DesktopWindowHandle desktopWindowHandle,
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

  void                DesktopEventMouseWheel( const int ticks )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataMouseWheel data;
    data.mDelta = ticks;
    sEventQueue.QueuePush( DesktopEventType::MouseWheel, &data, sizeof( data ) );
  }

  void                DesktopEventMouseMove( const DesktopWindowHandle desktopWindowHandle,
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

  void                DesktopEventKeyState( const Key key, const bool down )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataKeyState data;
    data.mDown = down;
    data.mKey = key;
    sEventQueue.QueuePush( DesktopEventType::KeyState, &data, sizeof( data ) );
  }

  void                DesktopEventKeyInput( const Codepoint codepoint )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataKeyInput data;
    data.mCodepoint = codepoint;
    sEventQueue.QueuePush( DesktopEventType::KeyInput, &data, sizeof( data ) );
  }

  void                DesktopEventMouseHoveredWindow( const DesktopWindowHandle desktopWindowHandle )
  {
    TAC_ASSERT( gThreadType == ThreadType::Main );
    DesktopEventDataCursorUnobscured data;
    data.mDesktopWindowHandle = desktopWindowHandle;
    sEventQueue.QueuePush( DesktopEventType::CursorUnobscured, &data, sizeof( data ) );
  }

  static void CreateRenderer( Errors& )
  {

#if defined _WIN32 || defined _WIN64 
    const String defaultRendererName = Render::RendererNameDirectX11;
#else
    const String defaultRendererName = RendererNameVulkan;
#endif
    if( const Render::RendererFactory* factory = Render::RendererFactoriesFind( defaultRendererName ) )
    {
      factory->mCreateRenderer();
      return;
    }

    Render::RendererFactory& factory = *Render::RendererRegistry().begin();
    factory.mCreateRenderer();
  }

  void                DesktopAppInit( PlatformSpawnWindow platformSpawnWindow,
                                      PlatformDespawnWindow platformDespawnWindow,
                                      PlatformGetMouseHoveredWindow platformGetMouseHoveredWindow,
                                      PlatformFrameBegin platformFrameBegin,
                                      PlatformFrameEnd platformFrameEnd,
                                      PlatformWindowMoveControls platformWindowMoveControls,
                                      PlatformWindowResizeControls platformWindowResizeControls,
                                      Errors& errors )
  {
    sEventQueue.Init();

    ExecutableStartupInfo info;
    info.Init( errors );
    TAC_HANDLE_ERROR( errors );

    String appDataPath;
    bool appDataPathExists;
    OSGetApplicationDataPath( appDataPath, errors );
    OSDoesFolderExist( appDataPath, appDataPathExists, errors );
    TAC_HANDLE_ERROR( errors );
    TAC_ASSERT( appDataPathExists );

    String appName = info.mAppName;
    String studioPath = appDataPath + "\\" + info.mStudioName + "\\";
    String prefPath = studioPath + appName;

    OSCreateFolderIfNotExist( studioPath, errors );
    TAC_HANDLE_ERROR( errors );

    OSCreateFolderIfNotExist( prefPath, errors );
    TAC_HANDLE_ERROR( errors );

    String workingDir;
    OSGetWorkingDir( workingDir, errors );
    TAC_HANDLE_ERROR( errors );

    sPlatformSpawnWindow = platformSpawnWindow;
    sPlatformDespawnWindow = platformDespawnWindow;
    sPlatformGetMouseHoveredWindow = platformGetMouseHoveredWindow;
    sPlatformFrameEnd = platformFrameEnd;
    sPlatformFrameBegin = platformFrameBegin;
    sPlatformWindowMoveControls = platformWindowMoveControls;
    sPlatformWindowResizeControls = platformWindowResizeControls;

    sProjectInit = info.mProjectInit;
    sProjectUpdate = info.mProjectUpdate;
    sProjectUninit = info.mProjectUninit;

    // right place?
    ShellSetAppName( appName.c_str() );
    ShellSetPrefPath( prefPath.c_str() );
    ShellSetInitialWorkingDir( workingDir );

    CreateRenderer( errors );
    Render::Init( errors );
    TAC_HANDLE_ERROR( errors );

  }

  void                DesktopAppRun( Errors& errors )
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

  void                DesktopAppUpdate( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    DesktopAppUpdateWindowRequests();
    DesktopAppUpdateMoveResize();
  }

  void                DesktopAppResizeControls( const DesktopWindowHandle& desktopWindowHandle, int edgePx )
  {
    RequestResize* request = &sRequestResize[ ( int )desktopWindowHandle ];
    request->mEdgePx = edgePx;
    request->mRequested = true;
  }

  void                DesktopAppMoveControls( const DesktopWindowHandle& desktopWindowHandle, DesktopWindowRect rect )
  {
    RequestMove* request = &sRequestMove[ ( int )desktopWindowHandle ];
    request->mRect = rect;
    request->mRequested = true;
  }

  void                DesktopAppMoveControls( const DesktopWindowHandle& desktopWindowHandle )
  {
    RequestMove* request = &sRequestMove[ ( int )desktopWindowHandle ];
    request->mRequested = true;
  }

  DesktopWindowHandle DesktopAppCreateWindow( int x, int y, int width, int height )
  {
    sWindowHandleLock.lock();
    const DesktopWindowHandle handle = { sDesktopWindowHandleIDs.Alloc() };
    WantSpawnInfo info;
    info.mX = x;
    info.mY = y;
    info.mWidth = width;
    info.mHeight = height;
    info.mHandle = handle;
    sWindowRequestsCreate.push_back( info );
    sWindowHandleLock.unlock();
    return handle;
  }

  void                DesktopAppDestroyWindow( DesktopWindowHandle desktopWindowHandle )
  {
    if( !desktopWindowHandle.IsValid() )
      return;
    sWindowHandleLock.lock();
    sDesktopWindowHandleIDs.Free( ( int )desktopWindowHandle );
    sWindowRequestsDestroy.push_back( desktopWindowHandle );
    sWindowHandleLock.unlock();
  }

  Errors*                        GetPlatformThreadErrors()
  {
    return &gPlatformThreadErrors;
  }

  Errors*                        GetLogicThreadErrors()
  {
    return &gLogicThreadErrors;

  }

  bool                           IsMainThread()
  {
    return gThreadType == ThreadType::Main;

  }
  bool                           IsLogicThread()
  {

    return gThreadType == ThreadType::Stuff;
  }

}
