#include "tac_desktop_app.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app_error_report.h"
#include "tac-desktop-app/desktop_app/tac_render_state.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event_handler.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_move.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_resize.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h"
#include "tac-engine-core/hid/controller/tac_controller_input.h"
#include "tac-engine-core/net/tac_net.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings_root.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/asset/tac_asset_hash_cache.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/os/tac_os.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <thread>
#endif

namespace Tac
{
  static DesktopEventHandler           sDesktopEventHandler;
#if TAC_SINGLE_THREADED()
  static Errors                        sAppErrors( Errors::kDebugBreaks );
#else
  static Errors                        sSysErrors( Errors::kDebugBreaks );
  static Errors                        sSimErrors( Errors::kDebugBreaks );
#endif
  static Errors                        gMainFunctionErrors( Errors::kDebugBreaks );
  static App*                          sApp;
  //static GameStateManager              sGameStateManager;
  static App::State                    sPrevState;
  static App::State                    sCurrState;
  static DesktopApp                    sDesktopApp;
  static SettingsRoot                  sSettingsRoot;
  static const bool                    sVerbose;
  static ThreadAllocator               sAppThreadAllocator;
  static Timestamp                     sRenderDelay( 0.0 );
  static int                           sNumRenderFramesSincePrevSimFrame{};


  // -----------------------------------------------------------------------------------------------

  static void DesktopAppDebugImGuiHoveredWindow()
  {
#if 0
    PlatformFns* platform { PlatformFns::GetInstance() };
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
  
  static auto ImGuiToPlatformMouseCursor( ImGuiMouseCursor imguiCursor ) -> PlatformMouseCursor
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

  void DesktopApp::Init( Errors& errors )
  {
    TAC_ASSERT( PlatformFns::GetInstance() );

    sApp = App::Create();

    sAppThreadAllocator.Init( 1024 * 1024 * 10 ); // 10MB
    FrameMemorySetThreadAllocator( &sAppThreadAllocator );

    Shell::sShellAppName = sApp->GetAppName();
    TAC_ASSERT( !Shell::sShellAppName.empty() );

    Shell::sShellStudioName = sApp->GetStudioName();
    TAC_ASSERT( !Shell::sShellStudioName.empty() );

    Shell::sShellPrefPath = TAC_CALL( OS::OSGetApplicationDataPath( errors ) );
    TAC_ASSERT( !Shell::sShellPrefPath.empty() );

    Shell::sShellInitialWorkingDir = FileSys::GetCurrentWorkingDirectory();
    TAC_ASSERT( !Shell::sShellInitialWorkingDir.empty() );

    // for macos standalone_sdl_vk_1_tri, appDataPath =
    //
    //     /Users/n473/Library/Application Support/Sleeping Studio/Vk Ex/
    //
    // for win32 project standalone_win_vk_1_tri, appDataPath =
    //
    //     C:\Users\Nate\AppData\Roaming + /Sleeping Studio + /Whatever bro
    TAC_RAISE_ERROR_IF( !FileSys::Exists( Shell::sShellPrefPath ), String()
                        + "app data path " + Shell::sShellPrefPath.u8string() + " doesnt exist" );

    const FileSys::Path logPath{ Shell::sShellPrefPath / ( Shell::sShellAppName + ".tac.log" ) };
    LogApi::LogSetPath( logPath );

    const FileSys::Path settingsPath{ Shell::sShellPrefPath / ( Shell::sShellAppName + ".tac.cfg" ) };
    TAC_CALL( sSettingsRoot.Init( settingsPath, errors ) );

    TAC_CALL( AssetHashCache::Init( errors ) );

    if( sVerbose )
      LogApi::LogMessagePrintLine( "DesktopApp::Init" );

    const Render::RenderApi::InitParams renderApiInitParams
    {
      .mShaderOutputPath { Shell::sShellPrefPath },
    };
    TAC_CALL( Render::RenderApi::Init( renderApiInitParams, errors ) );

    TAC_CALL( Shell::Init( errors ) );

#if TAC_FONT_ENABLED()
    TAC_CALL( FontApi::Init( errors ) );
#endif

    const ImGuiInitParams imguiInitParams
    {
      .mMaxGpuFrameCount { Render::RenderApi::GetMaxGPUFrameCount() },
      .mSettingsNode     { sSettingsRoot.GetRootNode() },
    };
    TAC_CALL( ImGuiInit( imguiInitParams, errors ) );

    DesktopEventApi::Init( &sDesktopEventHandler );

    // todo: this is ugly, fix it
    sApp->mSettingsNode = sSettingsRoot.GetRootNode();

    TAC_CALL( sApp->Init( errors ) );

    sCurrState = sApp->GameState_Create();
    sPrevState = sApp->GameState_Create();
  }

  void DesktopApp::Run( Errors& errors )
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


#if TAC_SINGLE_THREADED()
#else
    TAC_CALL( sSysThread.Init( errors ) );
    TAC_CALL( sSimThread.Init( errors ) );
#endif



#if TAC_SINGLE_THREADED()

    while( OS::OSAppIsRunning() )
    {


      // Win32FrameBegin polls wndproc
      TAC_CALL( platform->PlatformFrameBegin( errors ) );

      // Apply queued wndproc to saved keyboard/window state
      TAC_CALL( DesktopEventApi::Apply( errors ) );

      TAC_CALL( DesktopApp::Update( errors ) );
      TAC_CALL( platform->PlatformFrameEnd( errors ) );


      TAC_CALL( Network::NetApi::Update( errors ) );
      TAC_CALL( sSettingsRoot.Tick( errors ) );
      TAC_CALL( UpdateSimulation( errors ) );

#if TAC_DELETE_ME()
      static bool tempSlept;
      if( !tempSlept ) // temp temp temp
      {
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        tempSlept = true;
      }
#endif

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

  void DesktopApp::Update( Errors& )
  {
    DesktopAppUpdateMove();
    DesktopAppUpdateResize();
  }

  void DesktopApp::UpdateSimulation( Errors& errors )
  {
    if( !Timestep::Update() )
      return;

    // update at the end so that frameindex 0 has timestep 0

    TAC_PROFILE_BLOCK;
    ProfileSetGameFrame();

    LogApi::LogFlush();

    TAC_PROFILE_BLOCK_NAMED( "frame" );


    PlatformFns* platform{ PlatformFns::GetInstance() };

    const BeginFrameData data
    {
      .mElapsedSeconds      { Timestep::GetElapsedTime() },
      .mMouseHoveredWindow  { platform->PlatformGetMouseHoveredWindow() },
    };
    ImGuiBeginFrame( data );

    ControllerApi::UpdateJoysticks();

    TAC_CALL( sApp->Update( errors ) );

    TAC_CALL( ImGuiEndFrame( errors ) );

    AppKeyboardApiBackend::Sync();

    Swap( sPrevState, sCurrState );

    sApp->GameState_Update( sCurrState );
    if( sCurrState )
    {
      sCurrState->mFrameIndex = Timestep::GetElapsedFrames();
      sCurrState->mTimestamp = Timestep::GetElapsedTime();
      sCurrState->mTimepoint = Timestep::GetLastTick();
    }

    sNumRenderFramesSincePrevSimFrame = 0;
  }

  void DesktopApp::Render( Errors& errors )
  {
      TAC_PROFILE_BLOCK;

      Render::RenderApi::BeginRenderFrame( errors );
      TAC_ON_DESTRUCT( Render::RenderApi::EndRenderFrame( errors ) );

      //Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

      // Interpolate between game states and render
      //
      // Explanation:
      //   Suppose we have game states A, B, and C, except C doesn't exist yet, because C is in
      //   the future. If the current time is 25% of the way from B to C, we render (25% A + 75% B).
      //   This introduces some latency at the expense of misprediction (the alternative is 
      //   predicting 125% B)

      if( !sCurrState || !sPrevState || !sCurrState->mFrameIndex )
        return;

      TAC_ASSERT( sCurrState->mTimestamp != sPrevState->mTimestamp );

      const auto now{ Timepoint::Now() };

      dynmc float t{
        ( now - sPrevState->mTimepoint ) /
        ( sCurrState->mTimestamp - sPrevState->mTimestamp ) };

      // if currTime is inbetween pair.mOldState->mTimestamp and pair.mNewState->mTimestamp,
      // then we should instead of picking the two most recent simulation states, should pick
      // a pair thats one frame less recent
      TAC_ASSERT( t >= 0 );

      if( sVerbose )
        OS::OSDebugPrintLine( String() + "t before clamping: " + ToString( t ) );

      t = Min( t, 1.0f );

      const Timestamp interpolatedTimestamp{ Lerp( sPrevState->mTimestamp.mSeconds,
                                                   sCurrState->mTimestamp.mSeconds,
                                                   t ) };

      TAC_CALL( FontApi::UpdateGPU( errors ) );

      //ImGuiSimFrame* imguiSimFrame{ &sCurrState->mImGuiSimFrame };

      TAC_CALL( ImGuiPlatformRenderFrameBegin( // imguiSimFrame,
                                               errors ) );

      const App::RenderParams renderParams
      {
        .mOldState    { sPrevState }, // A
        .mNewState    { sCurrState }, // B
        .mT           { t }, // inbetween B and (future) C, but used to lerp A and B
        .mTimestamp   { interpolatedTimestamp },
      };

      if( sCurrState->mTimestamp.mSeconds > sRenderDelay )
      {
        TAC_CALL( sApp->Render( renderParams, errors ) );
        TAC_CALL( ImGuiPlatformRender( errors ) );
      }

      static PlatformMouseCursor oldCursor{ PlatformMouseCursor::kNone };
      const PlatformMouseCursor newCursor{ ImGuiToPlatformMouseCursor( ImGuiGlobals::Instance.mMouseCursor ) }; // imguiSimFrame->mCursor ) };
      if( oldCursor != newCursor )
      {
        oldCursor = newCursor;

        PlatformFns* platform{ PlatformFns::GetInstance() };
        platform->PlatformSetMouseCursor( newCursor );
        if( sVerbose )
          OS::OSDebugPrintLine( "set mouse cursor : " + ToString( ( int )newCursor ) );
      }

      for( ImGuiDesktopWindowImpl* desktopWindow : ImGuiGlobals::Instance.mDesktopWindows )
      //for( const ImGuiSimFrame::WindowSizeData& sizeData : imguiSimFrame->mWindowSizeDatas )
      {
        const WindowHandle windowHandle{ desktopWindow->mWindowHandle };

        if( desktopWindow->mRequestedPosition.HasValue() )
        //if( sizeData.mRequestedPosition.HasValue() )
        {
          const v2i windowPos{ AppWindowApi::GetPos( windowHandle ) };
          const v2i windowPosRequest{ desktopWindow->mRequestedPosition.GetValue() };
          if( windowPos != windowPosRequest )
            AppWindowApi::SetPos( windowHandle, windowPosRequest );
        }

        if( desktopWindow->mRequestedSize.HasValue() )
        {
          const v2i windowSize{ AppWindowApi::GetSize( windowHandle ) };
          const v2i windowSizeRequest{ desktopWindow->mRequestedSize.GetValue() };
          if( windowSize != windowSizeRequest )
            AppWindowApi::SetSize( windowHandle, windowSizeRequest );
        }
      }

      if( sCurrState->mTimestamp.mSeconds > sRenderDelay )
      {
        TAC_CALL( sApp->Present( errors ) );

        TAC_CALL( ImGuiPlatformPresent( // imguiSimFrame,
                                        errors ) );
      }
      //Render::FrameEnd();

      sNumRenderFramesSincePrevSimFrame++;
  }

  void DesktopApp::DebugImGui( Errors& errors )
  {
    if( !ImGuiCollapsingHeader( "DesktopAppDebugImGui" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    DesktopAppDebugImGuiHoveredWindow();

    PlatformFns* platform { PlatformFns::GetInstance() };
    platform->PlatformImGui( errors );
  }

  auto DesktopApp::GetMainErrors() -> Errors& { return gMainFunctionErrors; }

} // namespace Tac


