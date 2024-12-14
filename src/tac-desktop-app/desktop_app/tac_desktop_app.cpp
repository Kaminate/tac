#include "tac_desktop_app.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app_error_report.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app_renderers.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app_threads.h"
#include "tac-desktop-app/desktop_app/tac_render_state.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event_handler.h"
#include "tac-desktop-app/desktop_thread/tac_sim_thread.h"
#include "tac-desktop-app/desktop_thread/tac_sys_thread.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_move.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_resize.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/controller/tac_controller_input.h"
#include "tac-engine-core/hid/tac_keyboard_backend.h"
#include "tac-engine-core/hid/tac_sys_keyboard_api.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/net/tac_net.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings_root.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"
#include "tac-engine-core/asset/tac_asset_hash_cache.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/containers/tac_frame_vector.h"
#include "tac-std-lib/containers/tac_ring_buffer.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h" // Max
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/string/tac_string_view.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <thread>
#endif

namespace Tac
{
  static DesktopEventHandler           sDesktopEventHandler;

  static SysKeyboardApiBackend         sSysKeyboardBackend;
  static SimKeyboardApiBackend         sSimKeyboardBackend;
  static SysWindowApiBackend           sSysWindowBackend;
  static SimWindowApiBackend           sSimWindowBackend;

#if TAC_SINGLE_THREADED()
  static Errors                        sAppErrors( Errors::kDebugBreaks );
#else
  static Errors                        sSysErrors( Errors::kDebugBreaks );
  static Errors                        sSimErrors( Errors::kDebugBreaks );
#endif
  static Errors                        gMainFunctionErrors( Errors::kDebugBreaks );

  static App*                          sApp;

  static GameStateManager              sGameStateManager;
  static DesktopApp                    sDesktopApp;

  static SettingsRoot                  sSettingsRoot;
  static const bool                    sVerbose;

  static ThreadAllocator               sAppThreadAllocator;



  // -----------------------------------------------------------------------------------------------



  static void         DesktopAppDebugImGuiHoveredWindow()
  {
    PlatformFns* platform { PlatformFns::GetInstance() };
#if 0
    const WindowHandle hoveredHandle { platform->PlatformGetMouseHoveredWindow() };
    const DesktopWindowState* hovered { hoveredHandle.GetDesktopWindowState() };
    if( !hovered )
    {
      ImGuiText( "Hovered window: <none>" );
      return;
    }

    const ShortFixedString text{ ShortFixedString::Concat(
      "Hovered window: ",
      ToString( hoveredHandle.GetIndex() ),
      " ",
      hovered->mName ) };
    ImGuiText( text );
#endif
  }
  
  static PlatformMouseCursor ImGuiToPlatformMouseCursor( ImGuiMouseCursor imguiCursor )
  {
    switch( imguiCursor )
    {
    case ImGuiMouseCursor::kNone:        return PlatformMouseCursor::kNone;
    case ImGuiMouseCursor::kArrow:       return PlatformMouseCursor::kArrow;
    case ImGuiMouseCursor::kResizeNS:    return PlatformMouseCursor::kResizeNS;
    case ImGuiMouseCursor::kResizeEW:    return PlatformMouseCursor::kResizeEW;
    case ImGuiMouseCursor::kResizeNE_SW: return PlatformMouseCursor::kResizeNE_SW;
    case ImGuiMouseCursor::kResizeNW_SE: return PlatformMouseCursor::kResizeNW_SE;
    default: TAC_ASSERT_INVALID_CASE( imguiCursor ) ; return {};
    }
  }

  // -----------------------------------------------------------------------------------------------

  void                DesktopApp::Init( Errors& errors )
  {
    TAC_ASSERT( PlatformFns::GetInstance() );

    sApp = App::Create();

    DesktopAppThreads::SetType( DesktopAppThreads::ThreadType::App );

    sAppThreadAllocator.Init( 1024 * 1024 * 10 ); // 10MB
    FrameMemorySetThreadAllocator( &sAppThreadAllocator );

    sShellAppName = sApp->GetAppName();
    TAC_ASSERT( !sShellAppName.empty() );

    sShellStudioName = sApp->GetStudioName();
    TAC_ASSERT( !sShellStudioName.empty() );

    sShellPrefPath = TAC_CALL( OS::OSGetApplicationDataPath( errors ) );
    TAC_ASSERT( !sShellPrefPath.empty() );

    sShellInitialWorkingDir = FileSys::GetCurrentWorkingDirectory();
    TAC_ASSERT( !sShellInitialWorkingDir.empty() );

    // for macos standalone_sdl_vk_1_tri, appDataPath =
    //
    //     /Users/n473/Library/Application Support/Sleeping Studio/Vk Ex/
    //
    // for win32 project standalone_win_vk_1_tri, appDataPath =
    //
    //     C:\Users\Nate\AppData\Roaming + /Sleeping Studio + /Whatever bro
    TAC_RAISE_ERROR_IF( !FileSys::Exists( sShellPrefPath ), String()
                        + "app data path " + sShellPrefPath.u8string() + " doesnt exist" );

    const FileSys::Path logPath{ sShellPrefPath / ( sShellAppName + ".tac.log" ) };
    LogApi::LogSetPath( logPath );

    const FileSys::Path settingsPath{ sShellPrefPath / ( sShellAppName + ".tac.cfg" ) };
    TAC_CALL( sSettingsRoot.Init( settingsPath, errors ) );

    TAC_CALL( AssetHashCache::Init( errors ) );

    if( sApp->IsRenderEnabled() )
    {
      TAC_CALL( DesktopInitRendering( errors ) );
    }

    if( sVerbose )
      LogApi::LogMessagePrintLine( "DesktopApp::Init" );

    TAC_CALL( ShellInit( errors ) );

#if TAC_FONT_ENABLED()
    TAC_CALL( FontApi::Init( errors ) );
#endif
  }

  void                DesktopApp::Run( Errors& errors )
  {
    PlatformFns* platform{ PlatformFns::GetInstance() };

#if TAC_SINGLE_THREADED()
#else
    SimThread sSimThread
    {
      .mApp              { sApp },
      .mErrors           { &sSimErrors },
      .sGameStateManager { &sGameStateManager },
      .sWindowApi        { sSimWindowApi },
      .sKeyboardApi      { sSimKeyboardApi},
      .mSettingsRoot     { &sSettingsRoot },
    };

    SysThread sSysThread
    {
      .mApp              { sApp },
      .mErrors           { &sSysErrors },
      .mGameStateManager { &sGameStateManager },
      .mWindowApi        { sSysWindowApi },
      .AppKeyboardApi::      { sSysKeyboardApi },
    };
#endif


    const Render::RenderApi::InitParams renderApiInitParams
    {
      .mShaderOutputPath { sShellPrefPath },
    };
    TAC_CALL( Render::RenderApi::Init( renderApiInitParams, errors ) );

    const ImGuiInitParams imguiInitParams
    {
      .mMaxGpuFrameCount { Render::RenderApi::GetMaxGPUFrameCount() },
      .mSettingsNode     { sSettingsRoot.GetRootNode() },
    };
    TAC_CALL( ImGuiInit( imguiInitParams, errors ) );

    sDesktopEventHandler.mKeyboardBackend = &sSysKeyboardBackend;
    sDesktopEventHandler.mWindowBackend = &sSysWindowBackend;
    DesktopEventApi::Init( &sDesktopEventHandler );

    // this is kinda hacky
    DesktopAppThreads::SetType( DesktopAppThreads::ThreadType::Sys );

#if TAC_SINGLE_THREADED()
#else
    TAC_CALL( sSysThread.Init( errors ) );
    TAC_CALL( sSimThread.Init( errors ) );
#endif

    // todo: this is ugly, fix it
    sApp->mSettingsNode = sSettingsRoot.GetRootNode();

    TAC_CALL( sApp->Init( errors ) );


#if TAC_SINGLE_THREADED()

    while( OS::OSAppIsRunning() )
    {


      // Win32FrameBegin polls wndproc
      TAC_CALL( platform->PlatformFrameBegin( errors ) );

      // Apply queued wndproc to saved keyboard/window state
      TAC_CALL( DesktopEventApi::Apply( errors ) );

      TAC_CALL( DesktopApp::Update( errors ) );
      TAC_CALL( platform->PlatformFrameEnd( errors ) );

      TAC_CALL( sSysWindowBackend.Sync( errors ) );
      TAC_CALL( Network::NetApi::Update( errors ) );
      TAC_CALL( sSettingsRoot.Tick( errors ) );
      TAC_CALL( UpdateSimulation( errors ) );
      TAC_CALL( Render( errors ) );

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage
    }

    sApp->Uninit( errors );
    TAC_DELETE sApp;
    sApp = nullptr;

#else
    std::thread logicThread( &SimThread::Update, sSimThread, std::ref( SSimErrors ) );
    sSysThread.mApp = sApp;
    sSysThread.mErrors = &sSysErrors;
    sSysThread.Update( sSysErrors );
    logicThread.join();

    sSysThread.Uninit();
    sSimThread.Uninit();
#endif

    ImGuiUninit();

    DesktopAppErrorReport errorReport;
#if TAC_SINGLE_THREADED()
    errorReport.Add( "App Thread", &sAppErrors );
#else
    errorReport.Add( "Sys Thread", &sSysErrors );
    errorReport.Add( "Sim Thread", &SSimErrors );
#endif
    errorReport.Add( "Main Function", &gMainFunctionErrors );
    errorReport.Report();
  }



  void                DesktopApp::Update( Errors& errors )
  {
    DesktopAppUpdateMove();
    DesktopAppUpdateResize();
  }

  void                DesktopApp::UpdateSimulation(Errors&errors)
  {
    if( !Timestep::Update() )
      return;

    // update at the end so that frameindex 0 has timestep 0

    TAC_PROFILE_BLOCK;
    ProfileSetGameFrame();

    LogApi::LogFlush();

    // imo, the best time to pump the message queue would be right before simulation update
    // because it reduces input-->sim latency.
    // (ignore input-->render latency because of interp?)
    // So maybe wndproc should be moved here from the platform thread, and Render::SubmitFrame
    // and Render::RenderFrame should be rearranged
    TAC_PROFILE_BLOCK_NAMED( "frame" );

    sSimWindowBackend.Sync();
    sSimKeyboardBackend.Sync();

    PlatformFns* platform{ PlatformFns::GetInstance() };

    const BeginFrameData data
    {
      .mElapsedSeconds      { Timestep::GetElapsedTime() },
      .mMouseHoveredWindow  { platform->PlatformGetMouseHoveredWindow() },
    };
    ImGuiBeginFrame( data );

    Controller::UpdateJoysticks();

    TAC_CALL( sApp->Update( errors ) );

    TAC_CALL( ImGuiEndFrame( errors ) );

    App::IState* gameState{ sApp->GetGameState() };
    if( !gameState )
      gameState = TAC_NEW App::IState;

    gameState->mFrameIndex = Timestep::GetElapsedFrames();
    gameState->mTimestamp = Timestep::GetElapsedTime();
    gameState->mTimepoint = Timestep::GetLastTick();
    gameState->mImGuiSimFrame = ImGuiGetSimFrame();

    sGameStateManager.Enqueue( gameState );
  }

  void                DesktopApp::Render( Errors& errors )
  {
      TAC_PROFILE_BLOCK;

      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      TAC_CALL( renderDevice->Update( errors ) );

      // Interpolate between game states and render
      //
      // Explanation:
      //   Suppose we have game states A, B, and C, except C doesn't exist yet, because C is in
      //   the future. If the current time is 25% of the way from B to C, we render (25% A + 75% B).
      //   This introduces some latency at the expense of misprediction (the alternative is 
      //   predicting 125% B)

      GameStateManager::Pair pair{ sGameStateManager.Dequeue() };
      if( !pair.IsValid() )
        return;

      const TimestampDifference dt{ pair.mNewState->mTimestamp - pair.mOldState->mTimestamp };
      TAC_ASSERT( dt.mSeconds != 0 );

      const Timepoint prevTime{ pair.mNewState->mTimepoint };
      const Timepoint currTime{ Timepoint::Now() };

      dynmc float t{ ( currTime - prevTime ) / dt };

      // if currTime is inbetween pair.mOldState->mTimestamp and pair.mNewState->mTimestamp,
      // then we should instead of picking the two most recent simulation states, should pick
      // a pair thats one frame less recent
      TAC_ASSERT( t >= 0 );

      if( sVerbose )
        OS::OSDebugPrintLine( String() + "t before clamping: " + ToString( t ) );

      t = Min( t, 1.0f );

      const Timestamp interpolatedTimestamp{ Lerp( pair.mOldState->mTimestamp.mSeconds,
                                                   pair.mNewState->mTimestamp.mSeconds,
                                                   t ) };

      TAC_CALL( FontApi::UpdateGPU( errors ) );

      ImGuiSimFrame* imguiSimFrame{ &pair.mNewState->mImGuiSimFrame };

      TAC_CALL( ImGuiPlatformRenderFrameBegin( imguiSimFrame, errors ) );

      const App::RenderParams renderParams
      {
        .mOldState    { pair.mOldState }, // A
        .mNewState    { pair.mNewState }, // B
        .mT           { t }, // inbetween B and (future) C, but used to lerp A and B
        .mTimestamp   { interpolatedTimestamp },
      };
      TAC_CALL( sApp->Render( renderParams, errors ) );

      //const ImGuiSysDrawParams imguiDrawParams
      //{
      //  .mSimFrameDraws { &pair.mNewState->mImGuiDraws },
      //  .mWindowApi     { windowApi },
      //  .mTimestamp     { interpolatedTimestamp },
      //};

      TAC_CALL( ImGuiPlatformRender( imguiSimFrame, errors ) );

      static PlatformMouseCursor oldCursor{ PlatformMouseCursor::kNone };
      const PlatformMouseCursor newCursor{
        ImGuiToPlatformMouseCursor( imguiSimFrame->mCursor ) };
      if( oldCursor != newCursor )
      {
        oldCursor = newCursor;

        PlatformFns* platform{ PlatformFns::GetInstance() };
        platform->PlatformSetMouseCursor( newCursor );
        if( sVerbose )
          OS::OSDebugPrintLine( "set mouse cursor : " + ToString( ( int )newCursor ) );
      }

      for( const ImGuiSimFrame::WindowSizeData& sizeData : imguiSimFrame->mWindowSizeDatas )
      {
        if( sizeData.mRequestedPosition.HasValue() )
        {
          const v2i windowPos{ AppWindowApi::GetPos( sizeData.mWindowHandle ) };
          const v2i windowPosRequest{ sizeData.mRequestedPosition.GetValue() };
          if( windowPos != windowPosRequest )
            AppWindowApi::SetPos( sizeData.mWindowHandle, windowPosRequest );
        }

        if( sizeData.mRequestedSize.HasValue() )
        {
          const v2i windowSize{ AppWindowApi::GetSize( sizeData.mWindowHandle ) };
          const v2i windowSizeRequest{ sizeData.mRequestedSize.GetValue() };
          if( windowSize != windowSizeRequest )
            AppWindowApi::SetSize( sizeData.mWindowHandle, windowSizeRequest );
        }
      }

      TAC_CALL( sApp->Present( errors ) );

      TAC_CALL( ImGuiPlatformPresent( imguiSimFrame, errors ) );
      //Render::FrameEnd();
  }

  void                DesktopApp::DebugImGui( Errors& errors )
  {
    if( !ImGuiCollapsingHeader( "DesktopAppDebugImGui" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    DesktopAppDebugImGuiHoveredWindow();

    PlatformFns* platform { PlatformFns::GetInstance() };
    platform->PlatformImGui( errors );
  }

  Errors&             DesktopApp::GetMainErrors() { return gMainFunctionErrors; }

} // namespace Tac


