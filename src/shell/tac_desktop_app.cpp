#include "src/shell/tac_desktop_app.h" // self-include

#include "src/common/containers/tac_fixed_vector.h"
#include "src/common/containers/tac_frame_vector.h"
#include "src/common/containers/tac_ring_buffer.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_font.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/identifier/tac_id_collection.h"
#include "src/common/input/tac_controller_input.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/net/tac_net.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/string/tac_string.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/system/tac_os.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"
#include "src/shell/tac_register_renderers.h"
#include "src/space/tac_space.h"

#include <mutex>
#include <thread> // std::this_thread
#include <type_traits>

namespace Tac
{
  enum class ThreadType
  {
    Unknown,
    Main,
    Logic
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
    const char*         mName = nullptr;
    int                 mX = 0;
    int                 mY = 0;
    int                 mWidth = 0;
    int                 mHeight = 0;
  };

  struct DesktopEventQueueImpl
  {
    void       Init();

    template< typename T >
    void       QueuePush( DesktopEventType type, const T* t )
    {
      static_assert( std::is_trivially_copyable_v<T> );
      QueuePush( type, t, sizeof( T ) );
    }

    bool       QueuePop( void*, int );
    bool       Empty();

  private:
    void       QueuePush( DesktopEventType, const void*, int );
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

  typedef FixedVector< WantSpawnInfo, kDesktopWindowCapacity >       WindowRequestsCreate;
  typedef FixedVector< DesktopWindowHandle, kDesktopWindowCapacity > WindowRequestsDestroy;

  static Errors                        gPlatformThreadErrors( Errors::Flags::kDebugBreakOnAppend );
  static Errors                        gLogicThreadErrors( Errors::Flags::kDebugBreakOnAppend );
  static Errors                        gMainFunctionErrors( Errors::Flags::kDebugBreakOnAppend );
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
  //static ThreadAllocator               sAllocatorLogicThread;
  //static ThreadAllocator               sAllocatorMainThread;
  static DesktopEventQueueImpl         sEventQueue;
  static RequestMove                   sRequestMove[ kDesktopWindowCapacity ];
  static RequestResize                 sRequestResize[ kDesktopWindowCapacity ];
  thread_local ThreadType              gThreadType = ThreadType::Unknown;

  ExecutableStartupInfo ExecutableStartupInfo::sInstance;

  //void RegisterRenderers();

  static const char* GetDefaultRendererName()
  {
#if defined _WIN32 || defined _WIN64 
    return Render::RendererNameDirectX11;
#else
    return Render::RendererNameVulkan;
#endif
  }

  static const Render::RendererFactory* GetRendererFromSettings(Errors& errors)
  {
    String chosenRendererName = "";
    Json* rendererJson = SettingsGetJson( "chosen_renderer" );
    if( rendererJson->mType != JsonType::String )
    {
      chosenRendererName = SettingsGetString( "chosen_renderer", "" );
      SettingsFlush( errors );
    }

    const Render::RendererFactory* chosenFactory = Render::RendererFactoriesFind( chosenRendererName );
    return chosenFactory;
  }

  static const Render::RendererFactory* GetRendererPreferredPreprocessorVk()
  {
    #if TAC_PREFER_RENDERER_VK
    const Render::RendererFactory* factory = Render::RendererFactoriesFind( Render::RendererNameVulkan );
    return factory;
    #endif
    return nullptr;
  }

  static const Render::RendererFactory* GetFirstRendererFactory()
  {
    for( Render::RendererFactory& factory : Render::RendererRegistry() )
        return &factory;
    return nullptr;
  }

  static const Render::RendererFactory* GetRendererFactoryDefault()
  {
    const String defaultRendererName = GetDefaultRendererName();
    return Render::RendererFactoriesFind( defaultRendererName );
  }

  static const Render::RendererFactory* GetRendererFactory( Errors& errors )
  {
    for (const Render::RendererFactory* settingsFactory : {
      GetRendererFromSettings(errors),
      GetRendererPreferredPreprocessorVk(),
      GetRendererFactoryDefault(),
      GetFirstRendererFactory() })
    {
      if (settingsFactory)
        return settingsFactory;
    }

    return nullptr;
  }

  
  static void CreateRenderer( Errors& errors )
  {
    const Render::RendererFactory* factory = GetRendererFactory( errors );
    TAC_HANDLE_ERROR( errors );
    TAC_RAISE_ERROR_IF( !factory, "Failed to create renderer, are any factories registered?", errors );
    factory->mCreateRenderer();
  }

  static void DesktopAppUpdateWindowRequests()
  {
    sWindowHandleLock.lock();
    WindowRequestsCreate requestsCreate = sWindowRequestsCreate;
    WindowRequestsDestroy requestsDestroy = sWindowRequestsDestroy;
    sWindowRequestsCreate.clear();
    sWindowRequestsDestroy.clear();
    sWindowHandleLock.unlock();

    for( const WantSpawnInfo& info : requestsCreate )
      sPlatformSpawnWindow( info.mHandle,
                            info.mName,
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
    gThreadType = ThreadType::Logic;
    //sAllocatorLogicThread.Init( 1024 * 1024 * 10 );
    //FrameMemorySetThreadAllocator( &sAllocatorLogicThread );
    FrameMemoryInitThreadAllocator(  1024 * 1024 * 10  );

    ShellInit( errors );
    TAC_HANDLE_ERROR( errors );

    FontApi::Init( errors );
    TAC_HANDLE_ERROR( errors );

    ImGuiInit();
    SpaceInit();

    // ensure data path folder exists
    {
      
      Filesystem::Path dataPath = OS::OSGetApplicationDataPath( errors );
      TAC_HANDLE_ERROR( errors );

      const bool dataPathExists = Filesystem::Exists( dataPath );
      TAC_ASSERT( dataPathExists );

      // ???
      //OS::OSCreateFolderIfNotExist( dataPath, errors );
      TAC_HANDLE_ERROR( errors );

      ++asdf;
    }

    if(sProjectInit)
      sProjectInit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  static void LogicThreadUninit()
  {
    if(sProjectUninit)
      sProjectUninit( gLogicThreadErrors );

    ImGuiUninit();
    if( gLogicThreadErrors )
      OS::OSAppStopRunning();
    Render::SubmitFinish();
  }

  static void LogicThread()
  {
    Errors& errors = gLogicThreadErrors;
    TAC_ON_DESTRUCT( LogicThreadUninit() );
    LogicThreadInit( errors );
    TAC_HANDLE_ERROR( errors );

    while( OS::OSAppIsRunning() )
    {
      TAC_PROFILE_BLOCK;
      ProfileSetGameFrame();

      SettingsTick( errors );
      TAC_HANDLE_ERROR( errors );

      if( Net::Instance )
      {
        Net::Instance->Update( errors );
        TAC_HANDLE_ERROR( errors );
      }

      {
        TAC_PROFILE_BLOCK_NAMED( "update timer" );

        ShellTimerUpdate();
#if 1
        // fixed time step
        bool f = ShellTimerFrame();
        if( !f )
          continue;
#else
        // ??? time step
        while( ShellTimerFrame() )
        {
        }
#endif
      }

      {
        DesktopEventApplyQueue();

        // To reduce input latency, update the game soon after polling the controller.

        KeyboardBeginFrame();
        ImGuiFrameBegin( ShellGetElapsedSeconds(),
          sPlatformGetMouseHoveredWindow() );

        Input::UpdateJoysticks();

        if( sProjectUpdate )
        {
          sProjectUpdate( errors );
          TAC_HANDLE_ERROR( errors );
        }

        ImGuiFrameEnd( errors );
        TAC_HANDLE_ERROR( errors );

        KeyboardEndFrame();
      }

      Render::SubmitFrame();

      DontMaxOutCpuGpuPowerUsage();
      ShellIncrementFrameCounter();
    }

  }

  static void PlatformThreadUninit()
  {
    if( gPlatformThreadErrors )
      OS::OSAppStopRunning();
    Render::RenderFinish();
  }

  static void PlatformThreadInit( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    gThreadType = ThreadType::Main;
    //sAllocatorMainThread.Init( 1024 * 1024 * 10 );
    //FrameMemorySetThreadAllocator( &sAllocatorMainThread );
    FrameMemoryInitThreadAllocator(  1024 * 1024 * 10  );
  }

  static void PlatformThread()
  {
    Errors& errors = gPlatformThreadErrors;
    TAC_ON_DESTRUCT( PlatformThreadUninit() );

    while( OS::OSAppIsRunning() )
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

  //int* WindowHandleIterator::begin()
  //{
  //  TAC_ASSERT( IsLogicThread() );
  //  sWindowHandleLock.lock();
  //  return sDesktopWindowHandleIDs.begin();
  //}

  //int* WindowHandleIterator::end()
  //{
  //  int* result = sDesktopWindowHandleIDs.end();
  //  sWindowHandleLock.unlock();
  //  return result;
  //}

  void DesktopEventQueueImpl::Init()
  {
    const int kQueueCapacity = 1024;
    mQueue.Init( kQueueCapacity );
  }

  void DesktopEventQueueImpl::QueuePush( DesktopEventType desktopEventType,
                                         const void* dataBytes,
                                         int dataByteCount )
  {
    std::lock_guard< std::mutex > lockGuard( mMutex );
    // Tac::WindowProc still spews out events while a popupbox is open
    if( mQueue.capacity() - mQueue.size() < ( int )sizeof( DesktopEventType ) + dataByteCount )
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
    ShortFixedString    mName;
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
    TAC_ASSERT( IsLogicThread() );
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
                                             data.mName,
                                             data.mX,
                                             data.mY,
                                             data.mW,
                                             data.mH );
        }
        desktopWindowState->mNativeWindowHandle = data.mNativeWindowHandle;
        desktopWindowState->mName = data.mName;
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
        KeyboardSetWMCharPressedHax(data.mCodepoint);
      } break;

      case DesktopEventType::KeyState:
      {
        DesktopEventDataKeyState data;
        sEventQueue.QueuePop( &data, sizeof( data ) );
        KeyboardSetIsKeyDown( data.mKey, data.mDown );
      } break;

      case DesktopEventType::MouseWheel:
      {
        DesktopEventDataMouseWheel data;
        sEventQueue.QueuePop( &data, sizeof( data ) );
        KeyboardMouseWheelEvent(data.mDelta);
      } break;

      case DesktopEventType::MouseMove:
      {
        DesktopEventDataMouseMove data;
        sEventQueue.QueuePop( &data, sizeof( data ) );
        const DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
        KeyboardSetScreenspaceCursorPos( v2(( float )desktopWindowState->mX + ( float )data.mX,
                                            ( float )desktopWindowState->mY + ( float )data.mY ) );
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
                                                const char* name,
                                                const int x,
                                                const int y,
                                                const int w,
                                                const int h )
  {
    TAC_ASSERT( IsMainThread() );
    const DesktopEventDataAssignHandle data
    {
      .mDesktopWindowHandle = desktopWindowHandle,
      .mNativeWindowHandle = nativeWindowHandle,
      .mName = name,
      .mX = x,
      .mY = y,
      .mW = w,
      .mH = h,
    };
    sEventQueue.QueuePush( DesktopEventType::WindowAssignHandle, &data );
  }

  void                DesktopEventMoveWindow( const DesktopWindowHandle desktopWindowHandle,
    const int x,
    const int y )
  {
    TAC_ASSERT( IsMainThread() );
    DesktopEventDataWindowMove data{ .mDesktopWindowHandle = desktopWindowHandle,.mX = x,.mY = y };
    sEventQueue.QueuePush( DesktopEventType::WindowMove, &data );
  }

  void                DesktopEventResizeWindow( const DesktopWindowHandle desktopWindowHandle,
    const int w,
    const int h )
  {
    TAC_ASSERT( IsMainThread() );
    DesktopEventDataWindowResize data{ .mDesktopWindowHandle = desktopWindowHandle, .mWidth = w,.mHeight = h };
    sEventQueue.QueuePush( DesktopEventType::WindowResize, &data );
  }

  void                DesktopEventMouseWheel( const int ticks )
  {
    TAC_ASSERT( IsMainThread() );
    DesktopEventDataMouseWheel data{ .mDelta = ticks };
    sEventQueue.QueuePush( DesktopEventType::MouseWheel, &data );
  }

  void                DesktopEventMouseMove( const DesktopWindowHandle desktopWindowHandle,
    const int x,
    const int y )
  {
    TAC_ASSERT( IsMainThread() );
    DesktopEventDataMouseMove data{ .mDesktopWindowHandle = desktopWindowHandle, .mX = x, .mY = y };
    sEventQueue.QueuePush( DesktopEventType::MouseMove, &data );
  }

  void                DesktopEventKeyState( const Key key, const bool down )
  {
    TAC_ASSERT( IsMainThread() );
    DesktopEventDataKeyState data{ .mKey = key, .mDown = down };
    sEventQueue.QueuePush( DesktopEventType::KeyState, &data );
  }

  void                DesktopEventKeyInput( const Codepoint codepoint )
  {
    TAC_ASSERT( IsMainThread() );
    DesktopEventDataKeyInput data{ .mCodepoint = codepoint };
    sEventQueue.QueuePush( DesktopEventType::KeyInput, &data );
  }

  void                DesktopEventMouseHoveredWindow( const DesktopWindowHandle desktopWindowHandle )
  {
    TAC_ASSERT( IsMainThread() );
    DesktopEventDataCursorUnobscured data{ .mDesktopWindowHandle = desktopWindowHandle };
    sEventQueue.QueuePush( DesktopEventType::CursorUnobscured, &data );
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
    PlatformThreadInit( errors );
    TAC_HANDLE_ERROR( errors );

    sEventQueue.Init();

    ExecutableStartupInfo info = ExecutableStartupInfo::Init();
    TAC_ASSERT( !info.mAppName.empty() );

    ExecutableStartupInfo::sInstance = info;

    // for macos standalone_sdl_vk_1_tri, appDataPath =
    //
    //     /Users/n473/Library/Application Support/Sleeping Studio/Vk Ex/
    //
    // for win32 project standalone_win_vk_1_tri, appDataPath =
    //
    //     C:\Users\Nate\AppData\Roaming + /Sleeping Studio + /Whatever bro
    Filesystem::Path appDataPath= OS::OSGetApplicationDataPath( errors );
    TAC_HANDLE_ERROR( errors );

    //OS::OSDoesFolderExist(appDataPath, exists, errors);
    //TAC_HANDLE_ERROR( errors );
    const bool exists = Filesystem::Exists( appDataPath );
    TAC_ASSERT(exists);
    TAC_RAISE_ERROR_IF(!exists, va("app data path %s doesnt exist", appDataPath.u8string().c_str()), errors);

    const Filesystem::Path workingDir = Filesystem::GetCurrentWorkingDirectory(); //OS::OSGetWorkingDir( errors );
    TAC_HANDLE_ERROR( errors );

    // Platform Callbacks
    sPlatformSpawnWindow = platformSpawnWindow;
    sPlatformDespawnWindow = platformDespawnWindow;
    sPlatformGetMouseHoveredWindow = platformGetMouseHoveredWindow;
    sPlatformFrameEnd = platformFrameEnd;
    sPlatformFrameBegin = platformFrameBegin;
    sPlatformWindowMoveControls = platformWindowMoveControls;
    sPlatformWindowResizeControls = platformWindowResizeControls;

    // Project Callbacks
    sProjectInit = info.mProjectInit;
    sProjectUpdate = info.mProjectUpdate;
    sProjectUninit = info.mProjectUninit;

    // right place?
    ShellSetAppName( info.mAppName.c_str() );
    ShellSetPrefPath( appDataPath );
    ShellSetInitialWorkingDir( workingDir );

    SettingsInit( errors );
    TAC_HANDLE_ERROR( errors );

    RegisterRenderers();
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
    UpdateTrackedWindows();
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

  DesktopWindowHandle DesktopAppCreateWindow( const char* name, int x, int y, int width, int height )
  {
    sWindowHandleLock.lock();
    const DesktopWindowHandle handle = { sDesktopWindowHandleIDs.Alloc() };
    WantSpawnInfo info = { .mHandle = handle,
                           .mName = name,
                           .mX = x,
                           .mY = y,
                           .mWidth = width,
                           .mHeight = height, };
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

  static void         DesktopAppReportError( const char* name, Errors& errors )
  {
    if( !errors )
      return;
    String s;
    s += "Errors in ";
    s += name;
    s += " ";
    s += errors.ToString();
    OS::OSDebugPopupBox( s );
  }

  void                DesktopAppReportErrors()
  {
    DesktopAppReportError( "Platform Thread", gPlatformThreadErrors );
    DesktopAppReportError( "Main Function", gMainFunctionErrors );
    DesktopAppReportError( "Logic Thread", gLogicThreadErrors );
  }

  Errors& GetMainErrors()           { return gMainFunctionErrors; }

  bool                           IsMainThread() { return gThreadType == ThreadType::Main; }

  bool                           IsLogicThread() { return gThreadType == ThreadType::Logic; }

}
