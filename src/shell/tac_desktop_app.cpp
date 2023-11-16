#include "src/shell/tac_desktop_app.h" // self-include

#include "src/common/containers/tac_fixed_vector.h"
#include "src/common/containers/tac_frame_vector.h"
#include "src/common/containers/tac_ring_buffer.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_font.h"
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
#include "src/shell/tac_desktop_app_renderers.h"
#include "src/space/tac_space.h"

import std; // mutex, thread, type_traits

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
    MouseButtonState,
    KeyInput,
    MouseWheel,
    MouseMove,
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

    template< typename T >
    T          QueuePop()
    {
      T t;
      QueuePop( &t, sizeof( T ) );
      return t;
    }

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

  using WindowRequestsCreate = FixedVector< PlatformSpawnWindowParams, kDesktopWindowCapacity >;
  using WindowRequestsDestroy = FixedVector< DesktopWindowHandle, kDesktopWindowCapacity >;

  static Errors                        gPlatformThreadErrors( Errors::Flags::kDebugBreakOnAppend );
  static Errors                        gLogicThreadErrors( Errors::Flags::kDebugBreakOnAppend );
  static Errors                        gMainFunctionErrors( Errors::Flags::kDebugBreakOnAppend );

  static PlatformFns*                  sPlatformFns;
  static ProjectFns*                   sProjectFns;

  static std::mutex                    sWindowHandleLock;
  static IdCollection                  sDesktopWindowHandleIDs( kDesktopWindowCapacity );
  static WindowRequestsCreate          sWindowRequestsCreate;
  static WindowRequestsDestroy         sWindowRequestsDestroy;
  static DesktopEventQueueImpl         sEventQueue;
  static RequestMove                   sRequestMove[ kDesktopWindowCapacity ];
  static RequestResize                 sRequestResize[ kDesktopWindowCapacity ];
  thread_local ThreadType              gThreadType = ThreadType::Unknown;

  ExecutableStartupInfo ExecutableStartupInfo::sInstance;


  static void DesktopAppUpdateWindowRequests(Errors&errors)
  {
    sWindowHandleLock.lock();
    WindowRequestsCreate requestsCreate = sWindowRequestsCreate;
    WindowRequestsDestroy requestsDestroy = sWindowRequestsDestroy;
    sWindowRequestsCreate.clear();
    sWindowRequestsDestroy.clear();
    sWindowHandleLock.unlock();

    for( const PlatformSpawnWindowParams& info : requestsCreate )
      sPlatformFns->PlatformSpawnWindow( info, errors );

    for( const DesktopWindowHandle desktopWindowHandle : requestsDestroy )
      sPlatformFns->PlatformDespawnWindow( desktopWindowHandle );
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
        sPlatformFns->PlatformWindowMoveControls( desktopWindowHandle, desktopWindowRect );
        sRequestMove[ i ] = RequestMove();
      }

      const RequestResize* requestResize = &sRequestResize[ i ];
      if( requestResize->mRequested )
      {
        sPlatformFns->PlatformWindowResizeControls( desktopWindowHandle, requestResize->mEdgePx );
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
    FrameMemoryInitThreadAllocator(  1024 * 1024 * 10  );

    ShellInit( errors );
    TAC_HANDLE_ERROR( errors );

    FontApi::Init( errors );
    TAC_HANDLE_ERROR( errors );

    ImGuiInit();
    SpaceInit();

    sProjectFns->ProjectInit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  static void LogicThreadUninit()
  {
    sProjectFns->ProjectUninit( gLogicThreadErrors );

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

      Network::NetApi::Update( errors );
      TAC_HANDLE_ERROR( errors );


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

        Keyboard::KeyboardBeginFrame();
        Mouse::MouseBeginFrame();

        const BeginFrameData data =
        {
          .mElapsedSeconds = ShellGetElapsedSeconds(),
          .mMouseHoveredWindow = sPlatformFns->PlatformGetMouseHoveredWindow(),
        };
        ImGuiBeginFrame( data );

        Controller::UpdateJoysticks();

        sProjectFns->ProjectUpdate( errors );
        TAC_HANDLE_ERROR( errors );

        ImGuiEndFrame( errors );
        TAC_HANDLE_ERROR( errors );

        Keyboard::KeyboardEndFrame();
        Mouse::MouseEndFrame();
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

      sPlatformFns->PlatformFrameBegin( errors );
      TAC_HANDLE_ERROR( errors );

      DesktopAppUpdate( errors );
      TAC_HANDLE_ERROR( errors );

      sPlatformFns->PlatformFrameEnd( errors );
      TAC_HANDLE_ERROR( errors );

      Render::RenderFrame( errors );
      TAC_HANDLE_ERROR( errors );

      DontMaxOutCpuGpuPowerUsage();
    }

  }

  // -----------------------------------------------------------------------------------------------

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

  bool DesktopEventQueueImpl::QueuePop( void* dataBytes, int dataByteCount )
  {
    std::lock_guard< std::mutex > lockGuard( mMutex );
    return mQueue.Pop( dataBytes, dataByteCount );
  }

  // -----------------------------------------------------------------------------------------------

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
    Keyboard::Key       mKey = Keyboard::Key::Count;
    bool                mDown = false;
  };

  struct DesktopEventDataMouseButtonState
  {
    Mouse::Button       mButton = Mouse::Button::Count;
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

  // -----------------------------------------------------------------------------------------------

  void                DesktopEventInit()
  {
    sEventQueue.Init();
  }

  void                DesktopEventApplyQueue()
  {
    TAC_ASSERT( IsLogicThread() );
    while( !sEventQueue.Empty() )
    {
      //DesktopEventType desktopEventType = {};
      //if( !sEventQueue.QueuePop( &desktopEventType, sizeof( DesktopEventType ) ) )
      //  break;
      const auto desktopEventType = sEventQueue.QueuePop<DesktopEventType>();

      switch( desktopEventType )
      {
      case DesktopEventType::WindowAssignHandle:
      {
        //DesktopEventDataAssignHandle data;
        //sEventQueue.QueuePop( &data, sizeof( data ) );
        const auto data = sEventQueue.QueuePop<DesktopEventDataAssignHandle>();
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
        desktopWindowState->mName = (StringView)data.mName;
        desktopWindowState->mWidth = data.mW;
        desktopWindowState->mHeight = data.mH;
        desktopWindowState->mX = data.mX;
        desktopWindowState->mY = data.mY;
      } break;

      case DesktopEventType::WindowMove:
      {
        const auto data= sEventQueue.QueuePop<DesktopEventDataWindowMove>();
        DesktopWindowState* state = GetDesktopWindowState( data.mDesktopWindowHandle );
        state->mX = data.mX;
        state->mY = data.mY;
      } break;

      case DesktopEventType::WindowResize:
      {
        const auto data= sEventQueue.QueuePop<DesktopEventDataWindowResize>();
        DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
        desktopWindowState->mWidth = data.mWidth;
        desktopWindowState->mHeight = data.mHeight;
        WindowGraphicsResize( data.mDesktopWindowHandle,
                              desktopWindowState->mWidth,
                              desktopWindowState->mHeight );
      } break;

      case DesktopEventType::KeyInput:
      {
        const auto data= sEventQueue.QueuePop<DesktopEventDataKeyInput>();
        Keyboard::KeyboardSetWMCharPressedHax(data.mCodepoint);
      } break;

      case DesktopEventType::KeyState:
      {
        const auto data = sEventQueue.QueuePop<DesktopEventDataKeyState>();
        Keyboard::KeyboardSetIsKeyDown( data.mKey, data.mDown );
      } break;

      case DesktopEventType::MouseButtonState:
      {
        const auto  data = sEventQueue.QueuePop<DesktopEventDataMouseButtonState>();
        Mouse::ButtonSetIsDown( data.mButton, data.mDown );
      } break;

      case DesktopEventType::MouseWheel:
      {
        const auto data = sEventQueue.QueuePop<DesktopEventDataMouseWheel>();
        Mouse::MouseWheelEvent(data.mDelta);
      } break;

      case DesktopEventType::MouseMove:
      {
        const auto data = sEventQueue.QueuePop<DesktopEventDataMouseMove>();
        const DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
        const v2 windowPos = desktopWindowState->GetPosV2();
        const v2 dataPos( ( float )data.mX, ( float )data.mY );
        const v2 pos = windowPos + dataPos;
        Mouse::SetScreenspaceCursorPos( pos );
      } break;

      case DesktopEventType::CursorUnobscured:
      {
        const auto data = sEventQueue.QueuePop<DesktopEventDataCursorUnobscured>();
        SetHoveredWindow( data.mDesktopWindowHandle );
      } break;

      default:
        TAC_ASSERT_INVALID_CASE( desktopEventType );
        break;
      }
    }
  }

  void                DesktopEventAssignHandle( const DesktopWindowHandle& desktopWindowHandle,
                                                const void* nativeWindowHandle,
                                                const char* name,
                                                const int x,
                                                const int y,
                                                const int w,
                                                const int h )
  {
    TAC_ASSERT( IsMainThread() );
    const DesktopEventDataAssignHandle data // is the equals operator defined?
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

  void                DesktopEventMoveWindow( const DesktopWindowHandle& desktopWindowHandle,
                                              const int x,
                                              const int y )
  {
    TAC_ASSERT( IsMainThread() );
    const DesktopEventDataWindowMove data
    {
      .mDesktopWindowHandle = desktopWindowHandle,
      .mX = x,
      .mY = y
    };
    sEventQueue.QueuePush( DesktopEventType::WindowMove, &data );
  }

  void                DesktopEventResizeWindow( const DesktopWindowHandle& desktopWindowHandle,
                                                const int w,
                                                const int h )
  {
    TAC_ASSERT( IsMainThread() );
    const DesktopEventDataWindowResize data
    {
      .mDesktopWindowHandle = desktopWindowHandle,
      .mWidth = w,
      .mHeight = h
    };
    sEventQueue.QueuePush( DesktopEventType::WindowResize, &data );
  }

  void                DesktopEventMouseWheel( const int ticks )
  {
    TAC_ASSERT( IsMainThread() );
    const DesktopEventDataMouseWheel data{ .mDelta = ticks };
    sEventQueue.QueuePush( DesktopEventType::MouseWheel, &data );
  }

  void                DesktopEventMouseMove( const DesktopWindowHandle& desktopWindowHandle,
                                             const int x,
                                             const int y )
  {
    TAC_ASSERT( IsMainThread() );
    const DesktopEventDataMouseMove data
    {
      .mDesktopWindowHandle = desktopWindowHandle,
      .mX = x,
      .mY = y
    };
    sEventQueue.QueuePush( DesktopEventType::MouseMove, &data );
  }

  void                DesktopEventKeyState( const Keyboard::Key& key, const bool down )
  {
    TAC_ASSERT( IsMainThread() );
    const DesktopEventDataKeyState data
    {
      .mKey = key,
      .mDown = down
    };
    sEventQueue.QueuePush( DesktopEventType::KeyState, &data );
  }

  void                DesktopEventMouseButtonState( const Mouse::Button& button, const bool down )
  {
    TAC_ASSERT( IsMainThread() );
    const DesktopEventDataMouseButtonState data
    {
      .mButton = button,
      .mDown = down
    };
    sEventQueue.QueuePush( DesktopEventType::MouseButtonState, &data );
  }

  void                DesktopEventKeyInput( const Codepoint& codepoint )
  {
    TAC_ASSERT( IsMainThread() );
    DesktopEventDataKeyInput data{ .mCodepoint = codepoint };
    sEventQueue.QueuePush( DesktopEventType::KeyInput, &data );
  }

  void                DesktopEventMouseHoveredWindow( const DesktopWindowHandle& desktopWindowHandle )
  {
    TAC_ASSERT( IsMainThread() );
    DesktopEventDataCursorUnobscured data{ .mDesktopWindowHandle = desktopWindowHandle };
    sEventQueue.QueuePush( DesktopEventType::CursorUnobscured, &data );
  }

  // -----------------------------------------------------------------------------------------------

  void                DesktopAppInit( PlatformFns* platformFns, Errors& errors )
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
    const Filesystem::Path appDataPath = OS::OSGetApplicationDataPath( errors );
    TAC_HANDLE_ERROR( errors );
    if( !Filesystem::Exists( appDataPath ) )
    {
      const String msg = "app data path " + appDataPath.u8string() + " doesnt exist";
      TAC_RAISE_ERROR( msg, errors );
    }

    const Filesystem::Path workingDir = Filesystem::GetCurrentWorkingDirectory();
    TAC_HANDLE_ERROR( errors );

    sPlatformFns = platformFns;
    sProjectFns = info.mProjectFns;

    // right place?
    ShellSetAppName( info.mAppName.c_str() );
    ShellSetPrefPath( appDataPath );
    ShellSetInitialWorkingDir( workingDir );

    SettingsInit( errors );
    TAC_HANDLE_ERROR( errors );

    DesktopInitRendering(errors);
    TAC_HANDLE_ERROR( errors );
  }

  void                DesktopAppRun( Errors& errors )
  {
    std::thread logicThread( LogicThread );
    PlatformThread();
    logicThread.join();
  }

  void                DesktopAppUpdate( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    DesktopAppUpdateWindowRequests(errors);
    DesktopAppUpdateMoveResize();
    UpdateTrackedWindows();
  }

  void                DesktopAppResizeControls( const DesktopWindowHandle& desktopWindowHandle, int edgePx )
  {
    sRequestResize[ ( int )desktopWindowHandle ] = RequestResize
    {
     .mRequested = true,
     .mEdgePx = edgePx,
    };
  }

  void                DesktopAppMoveControls( const DesktopWindowHandle& desktopWindowHandle,
                                              const DesktopWindowRect& rect )
  {
    sRequestMove[ ( int )desktopWindowHandle ] = RequestMove
    {
      .mRequested = true,
      .mRect = rect,
    };
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
    const PlatformSpawnWindowParams info =
    {
      .mHandle = handle,
      .mName = name,
      .mX = x,
      .mY = y,
      .mWidth = width,
      .mHeight = height,
    };
    sWindowRequestsCreate.push_back( info );
    sWindowHandleLock.unlock();
    return handle;
  }

  void                DesktopAppDestroyWindow( const DesktopWindowHandle& desktopWindowHandle )
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

  void                DesktopAppDebugImGui(Errors& errors)
  {
    if( !ImGuiCollapsingHeader("DesktopAppDebugImGui"))
      return;

    TAC_IMGUI_INDENT_BLOCK;

    DesktopWindowDebugImgui();

    {
      ShortFixedString text = "Hovered window: <none>";
      const DesktopWindowHandle hoveredHandle = sPlatformFns->PlatformGetMouseHoveredWindow();
      if( const DesktopWindowState* hovered = GetDesktopWindowState( hoveredHandle ) )
      {
        text = "Hovered window: ";
        text += ToString(hoveredHandle.GetIndex());
        text += " ";
        text += ( StringView )hovered->mName;
      }

      ImGuiText( text );
    }

    sPlatformFns->PlatformImGui(errors);
  }

  // -----------------------------------------------------------------------------------------------

  Errors&             GetMainErrors() { return gMainFunctionErrors; }

  bool                IsMainThread()  { return gThreadType == ThreadType::Main; }

  bool                IsLogicThread() { return gThreadType == ThreadType::Logic; }

  // -----------------------------------------------------------------------------------------------

  void ProjectFns::ProjectInit( Errors& ) { TAC_NO_OP; }
  void ProjectFns::ProjectUpdate( Errors& ) { TAC_NO_OP; }
  void ProjectFns::ProjectUninit( Errors& ) { TAC_NO_OP; }

  // -----------------------------------------------------------------------------------------------

  void PlatformFns::PlatformImGui( Errors& ) { TAC_NO_OP; };
  void PlatformFns::PlatformFrameBegin( Errors& ) { TAC_NO_OP; }
  void PlatformFns::PlatformFrameEnd( Errors& ) { TAC_NO_OP; }
  void PlatformFns::PlatformSpawnWindow( const PlatformSpawnWindowParams&, Errors& ) { TAC_NO_OP; }
  void PlatformFns::PlatformDespawnWindow( const DesktopWindowHandle& ) { TAC_NO_OP; }
  void PlatformFns::PlatformWindowMoveControls( const DesktopWindowHandle&,
                                                const DesktopWindowRect& )
  {
    TAC_NO_OP;
  }
  void PlatformFns::PlatformWindowResizeControls ( const DesktopWindowHandle&, int ) { TAC_NO_OP; }
  DesktopWindowHandle PlatformFns::PlatformGetMouseHoveredWindow() { TAC_NO_OP_RETURN( {} ); }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac
