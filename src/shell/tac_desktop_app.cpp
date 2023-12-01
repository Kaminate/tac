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
#include "src/shell/tac_desktop_event.h"
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

  //enum class DesktopEventType
  //{
  //  Unknown = 0,
  //  WindowAssignHandle,
  //  WindowResize,
  //  WindowMove,
  //  CursorUnobscured,
  //  KeyState,
  //  MouseButtonState,
  //  KeyInput,
  //  MouseWheel,
  //  MouseMove,
  //};

  //struct DesktopEventQueueImpl
  //{
  //  void       Init();

  //  template< typename T >
  //  void       QueuePush( DesktopEventType type, const T* t )
  //  {
  //    static_assert( std::is_trivially_copyable_v<T> );
  //    QueuePush( type, t, sizeof( T ) );
  //  }

  //  bool       QueuePop( void*, int );

  //  template< typename T >
  //  T          QueuePop()
  //  {
  //    T t;
  //    QueuePop( &t, sizeof( T ) );
  //    return t;
  //  }

  //  bool       Empty();

  //private:
  //  void       QueuePush( DesktopEventType, const void*, int );
  //  RingBuffer mQueue;
  //  std::mutex mMutex;
  //};

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

  static App*                          sApp;
  static bool                          sRenderEnabled;

  static std::mutex                    sWindowHandleLock;
  static IdCollection                  sDesktopWindowHandleIDs( kDesktopWindowCapacity );
  static WindowRequestsCreate          sWindowRequestsCreate;
  static WindowRequestsDestroy         sWindowRequestsDestroy;
  //static DesktopEventQueueImpl         sEventQueue;
  static RequestMove                   sRequestMove[ kDesktopWindowCapacity ];
  static RequestResize                 sRequestResize[ kDesktopWindowCapacity ];
  thread_local ThreadType              gThreadType = ThreadType::Unknown;

  // -----------------------------------------------------------------------------------------------

  App::App(const App::Config& config) : mConfig( config ){};

  // -----------------------------------------------------------------------------------------------

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

  // -----------------------------------------------------------------------------------------------

  static void LogicThreadInit( Errors& errors )
  {
    gThreadType = ThreadType::Logic;
    FrameMemoryInitThreadAllocator(  1024 * 1024 * 10  );

    TAC_CALL( ShellInit, errors );

    TAC_CALL( FontApi::Init, errors );

    ImGuiInit();
    SpaceInit();

    TAC_CALL( sApp->Init, errors );
  }

  static void LogicThreadUninit()
  {
    {
      sApp->Uninit( gLogicThreadErrors );
      TAC_DELETE sApp;
      sApp = nullptr;
    }

    ImGuiUninit();

    if( gLogicThreadErrors )
      OS::OSAppStopRunning();

    if( sRenderEnabled )
      Render::SubmitFinish();
  }

  static void LogicThread()
  {
    Errors& errors = gLogicThreadErrors;
    TAC_ON_DESTRUCT( LogicThreadUninit() );
    TAC_CALL( LogicThreadInit, errors );

    while( OS::OSAppIsRunning() )
    {
      TAC_PROFILE_BLOCK;
      ProfileSetGameFrame();

      TAC_CALL( SettingsTick, errors );

      TAC_CALL( Network::NetApi::Update, errors );


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

        TAC_CALL( sApp->Update, errors );

        TAC_CALL( ImGuiEndFrame, errors );

        Keyboard::KeyboardEndFrame();
        Mouse::MouseEndFrame();
      }

      if( sRenderEnabled )
        Render::SubmitFrame();

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage

      ShellIncrementFrameCounter();
    }

  }

  // -----------------------------------------------------------------------------------------------

  static void PlatformThreadUninit()
  {
    if( gPlatformThreadErrors )
      OS::OSAppStopRunning();

    if( sRenderEnabled )
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

      TAC_CALL( sPlatformFns->PlatformFrameBegin, errors );

      TAC_CALL( DesktopAppUpdate, errors );

      TAC_CALL( sPlatformFns->PlatformFrameEnd, errors );

      if( sRenderEnabled )
      {
        TAC_CALL( Render::RenderFrame, errors );
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage
    }

  }

  // -----------------------------------------------------------------------------------------------

  void                DesktopAppInit( PlatformFns* platformFns, Errors& errors )
  {
    TAC_CALL( PlatformThreadInit, errors );

    DesktopEventInit();

    sApp = App::Create();
    sRenderEnabled = !sApp->mConfig.mDisableRenderer;
    TAC_ASSERT( !sApp->mConfig.mName.empty() );

    sPlatformFns = platformFns;

    // right place?
    sShellAppName = sApp->mConfig.mName;
    sShellStudioName = sApp->mConfig.mStudioName;
    sShellPrefPath = TAC_CALL( OS::OSGetApplicationDataPath, errors );
    sShellInitialWorkingDir = Filesystem::GetCurrentWorkingDirectory();

    // for macos standalone_sdl_vk_1_tri, appDataPath =
    //
    //     /Users/n473/Library/Application Support/Sleeping Studio/Vk Ex/
    //
    // for win32 project standalone_win_vk_1_tri, appDataPath =
    //
    //     C:\Users\Nate\AppData\Roaming + /Sleeping Studio + /Whatever bro
    if( !Filesystem::Exists( sShellPrefPath ) )
    {
      const String msg = "app data path " + sShellPrefPath.u8string() + " doesnt exist";
      TAC_RAISE_ERROR( msg );
    }

    TAC_CALL( SettingsInit, errors );

    if( sRenderEnabled )
    {
      TAC_CALL( DesktopInitRendering, errors );
    }
  }

  void                DesktopAppRun( Errors& errors )
  {
    std::thread logicThread( LogicThread );
    PlatformThread();
    logicThread.join();
  }

  void                DesktopAppUpdate( Errors& errors )
  {
    TAC_CALL( DesktopAppUpdateWindowRequests,errors);
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

  DesktopWindowHandle DesktopAppCreateWindow( const DesktopAppCreateWindowParams& desktopParams )
  {
    sWindowHandleLock.lock();
    const DesktopWindowHandle handle = { sDesktopWindowHandleIDs.Alloc() };
    const PlatformSpawnWindowParams info =
    {
      .mHandle = handle,
      .mName = desktopParams.mName,
      .mX = desktopParams.mX,
      .mY = desktopParams.mY,
      .mWidth = desktopParams.mWidth,
      .mHeight = desktopParams.mHeight,
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

  static void         DesktopAppDebugImGuiHoveredWindow()
  {
    const DesktopWindowHandle hoveredHandle = sPlatformFns->PlatformGetMouseHoveredWindow();
    const DesktopWindowState* hovered = GetDesktopWindowState( hoveredHandle );
    if( !hovered )
    {
      ImGuiText( "Hovered window: <none>" );
      return;
    }

    const ShortFixedString text = ShortFixedString::Concat(
      "Hovered window: ",
      ToString( hoveredHandle.GetIndex() ),
      " ",
      hovered->mName );
    ImGuiText( text );
  }

  void                DesktopAppDebugImGui(Errors& errors)
  {
    if( !ImGuiCollapsingHeader("DesktopAppDebugImGui"))
      return;

    TAC_IMGUI_INDENT_BLOCK;

    DesktopWindowDebugImgui();

    DesktopAppDebugImGuiHoveredWindow();

    sPlatformFns->PlatformImGui(errors);
  }

  // -----------------------------------------------------------------------------------------------

  Errors&             GetMainErrors() { return gMainFunctionErrors; }

  bool                IsMainThread()  { return gThreadType == ThreadType::Main; }

  bool                IsLogicThread() { return gThreadType == ThreadType::Logic; }

  // -----------------------------------------------------------------------------------------------

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
