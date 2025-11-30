#include "tac_desktop_app.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app_error_report.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event_handler.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_move.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_resize.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/asset/tac_asset_hash_cache.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/controller/tac_controller_input.h"
#include "tac-engine-core/job/tac_job_queue.h"
#include "tac-engine-core/net/tac_net.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings_root.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <thread>
#endif

namespace Tac
{
  static DesktopEventHandler           sDesktopEventHandler;
  static Errors                        gMainFunctionErrors( Errors::kDebugBreaks );
  static App*                          sApp;
  static App::State                    sPrevState;
  static App::State                    sCurrState;
  static DesktopApp                    sDesktopApp;
  static SettingsRoot                  sSettingsRoot;
  static const bool                    sVerbose;
  static ThreadAllocator               sAppThreadAllocator;
  static GameTime                     sRenderDelay;
  static int                           sNumRenderFramesSincePrevSimFrame{};
  
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

  static void DesktopAppUpdateSimulation( Errors& errors )
  {
    if( !GameTimer::Update() )
      return;

    // update at the end so that GameFrame 0 has GameTimer 0

    TAC_PROFILE_BLOCK;
    ProfileSetGameFrame();

    LogApi::LogFlush();

    TAC_PROFILE_BLOCK_NAMED( "frame" );

    ImGuiBeginFrame( BeginFrameData
    {
      .mElapsedSeconds      { GameTimer::GetElapsedTime() },
      .mMouseHoveredWindow  { Platform::PlatformGetMouseHoveredWindow() },
    } );

    ControllerApi::UpdateJoysticks();

    TAC_CALL( sApp->Update( errors ) );

    TAC_CALL( ImGuiEndFrame( errors ) );

    AppKeyboardApiBackend::sGameKeyboardBackend.Sync();
    AppKeyboardApiBackend::sUIKeyboardBackend.Sync();

    Swap( sPrevState, sCurrState );

    sApp->GameState_Update( sCurrState );
    if( sCurrState )
    {
      sCurrState->mFrameIndex = GameTimer::GetElapsedFrames();
      sCurrState->mGameTime = GameTimer::GetElapsedTime();
      sCurrState->mRealTime = GameTimer::GetLastTick();
    }

    sNumRenderFramesSincePrevSimFrame = 0;
  }

  static void DesktopAppRender( Errors& errors )
  {
      TAC_PROFILE_BLOCK;

      Render::RenderApi::BeginRenderFrame( errors );
      TAC_ON_DESTRUCT( Render::RenderApi::EndRenderFrame( errors ) );

      // Interpolate between game states and render
      //
      // Explanation:
      //   Suppose we have game states A, B, and C, except C doesn't exist yet, because C is in
      //   the future. If the current time is 25% of the way from B to C, we render (25% A + 75% B).
      //   This introduces some latency at the expense of misprediction (the alternative is 
      //   predicting 125% B)

      if( !sCurrState || !sPrevState || !sCurrState->mFrameIndex )
        return;

      TAC_ASSERT( sCurrState->mGameTime != sPrevState->mGameTime );
      const auto now{ RealTime::Now() };
      dynmc float t{
        ( now - sPrevState->mRealTime ) /
        ( sCurrState->mGameTime - sPrevState->mGameTime ) };

      // if currTime is inbetween pair.mOldState->mGameTime and pair.mNewState->mGameTime,
      // then we should instead of picking the two most recent simulation states, should pick
      // a pair thats one frame less recent
      TAC_ASSERT( t >= 0 );

      if( sVerbose )
        OS::OSDebugPrintLine( String() + "t before clamping: " + ToString( t ) );

      t = Min( t, 1.0f );
      TAC_CALL( FontApi::UpdateGPU( errors ) );
      TAC_CALL( ImGuiPlatformRenderFrameBegin( errors ) );
      if( sCurrState->mGameTime.mSeconds > sRenderDelay )
      {
        TAC_CALL( sApp->Render(
          App::RenderParams
          {
            .mOldState    { sPrevState }, // A
            .mNewState    { sCurrState }, // B
            .mT           { t }, // inbetween B and (future) C, but used to lerp A and B
            .mGameTime   { Lerp( sPrevState->mGameTime.mSeconds, sCurrState->mGameTime.mSeconds, t ) },
          }, errors ) );
        TAC_CALL( ImGuiPlatformRender( errors ) );
      }

      static PlatformMouseCursor oldCursor{ PlatformMouseCursor::kNone };
      const PlatformMouseCursor newCursor{ ImGuiToPlatformMouseCursor( ImGuiGlobals::mMouseCursor ) };
      if( oldCursor != newCursor )
      {
        oldCursor = newCursor;
        Platform::PlatformSetMouseCursor( newCursor );
        if( sVerbose )
          OS::OSDebugPrintLine( "set mouse cursor : " + ToString( ( int )newCursor ) );
      }

      for( ImGuiDesktopWindowImpl* desktopWindow : ImGuiGlobals::mDesktopWindows )
      {
        if( desktopWindow->mRequestedPosition.HasValue() )
        {
          const v2i windowPos{ AppWindowApi::GetPos( desktopWindow->mWindowHandle ) };
          const v2i windowPosRequest{ desktopWindow->mRequestedPosition.GetValue() };
          if( windowPos != windowPosRequest )
          {
            AppWindowApi::SetPos( desktopWindow->mWindowHandle, windowPosRequest );
          }
        }

        if( desktopWindow->mRequestedSize.HasValue() )
        {
          const v2i windowSize{ AppWindowApi::GetSize( desktopWindow->mWindowHandle ) };
          const v2i windowSizeRequest{ desktopWindow->mRequestedSize.GetValue() };
          if( windowSize != windowSizeRequest )
            AppWindowApi::SetSize( desktopWindow->mWindowHandle, windowSizeRequest );
        }
      }

      if( sCurrState->mGameTime.mSeconds > sRenderDelay )
      {
        //TAC_CALL( sApp->Present( errors ) );
        TAC_CALL( ImGuiPlatformPresent( errors ) );
        TAC_CALL( AppWindowMgr::RenderPresent( errors ) );
      }

      sNumRenderFramesSincePrevSimFrame++;
  }

  static void DesktopAppInit( Errors& errors )
  {
    sApp = App::Create();
    sAppThreadAllocator.Init( 1024 * 1024 * 10 ); // 10MB
    FrameMemorySetThreadAllocator( &sAppThreadAllocator );
    Shell::sShellAppName = sApp->GetAppName();
    Shell::sShellStudioName = sApp->GetStudioName();
    Shell::sShellPrefPath = TAC_CALL( OS::OSGetApplicationDataPath( errors ) );
    TAC_ASSERT( !Shell::sShellAppName.empty() );
    TAC_ASSERT( !Shell::sShellStudioName.empty() );
    TAC_ASSERT( !Shell::sShellPrefPath.empty() );

    // macos appDataPath = /Users/Nate/Library/Application Support/Studio/Project
    // win32 appDataPath = C:/Users/Nate/AppData/Roaming/Studio/Project
    TAC_RAISE_ERROR_IF( !Shell::sShellPrefPath.Exists(),
                        String() + "app data path " + Shell::sShellPrefPath + " doesnt exist" );
    LogApi::LogSetPath( Shell::sShellPrefPath / ( Shell::sShellAppName + ".tac.log" ) );
    const UTF8Path settingsPath{ Shell::sShellPrefPath / ( Shell::sShellAppName + ".tac.cfg" ) };
    TAC_CALL( sSettingsRoot.Init( settingsPath, errors ) );
    Shell::sShellSettings = sSettingsRoot.GetRootNode();
    TAC_CALL( AssetHashCache::Init( errors ) );
    if( sVerbose )
      LogApi::LogMessagePrintLine( "DesktopApp::Init" );

    TAC_CALL( Render::RenderApi::Init(
      Render::RenderApi::InitParams
      {
        .mShaderOutputPath { Shell::sShellPrefPath },
      }, errors ) );
    Job::JobQueueInit();
    ModelAssetManager::Init();
    TAC_CALL( LocalizationLoad( "assets/localization.txt", errors ) );
    TAC_CALL( Render::DefaultCBufferPerFrame::Init( errors ) );
    TAC_CALL( Render::DefaultCBufferPerObject::Init( errors ) );
    TAC_CALL( Render::CBufferLights::Init( errors ) );
    TAC_CALL( UI2DCommonDataInit( errors ) );
    TAC_CALL( Debug3DCommonDataInit( errors ) );
#if TAC_FONT_ENABLED()
    TAC_CALL( FontApi::Init( errors ) );
#endif
    TAC_CALL( ImGuiInit(
      ImGuiInitParams
      {
        .mMaxGpuFrameCount { Render::RenderApi::GetMaxGPUFrameCount() },
        .mSettingsNode     { sSettingsRoot.GetRootNode() },
      }, errors ) );
    DesktopEventApi::Init( &sDesktopEventHandler );
    TAC_CALL( sApp->Init( errors ) );
    sCurrState = sApp->GameState_Create();
    sPrevState = sApp->GameState_Create();
  }

  static void DesktopAppUninit( Errors& errors )
  {
    sApp->Uninit( errors );
    TAC_DELETE sApp;
    sApp = nullptr;

    UI2DCommonDataUninit();
    Debug3DCommonDataUninit();

  #if TAC_FONT_ENABLED()
    FontApi::Uninit();
  #endif

    ModelAssetManager::Uninit();

    ImGuiUninit();
    Render::RenderApi::Uninit();
  }

  void DesktopApp::Run( Errors& errors )
  {
    TAC_ON_DESTRUCT( DesktopAppErrorReport::Report( &errors ) );
    TAC_CALL( DesktopAppInit( errors ) );
    while( OS::OSAppIsRunning() )
    {
      TAC_CALL( Platform::PlatformFrameBegin( errors ) ); // poll wndproc
      TAC_CALL( DesktopEventApi::Apply( errors ) ); // apply queued wndproc events to keyboard/window state
      TAC_CALL( DesktopApp::Update( errors ) );
      TAC_CALL( Platform::PlatformFrameEnd( errors ) );
      TAC_CALL( Network::NetApi::Update( errors ) );
      TAC_CALL( sSettingsRoot.Tick( errors ) );
      TAC_CALL( DesktopAppUpdateSimulation( errors ) );
      TAC_CALL( DesktopAppRender( errors ) );
      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage
    }

    TAC_CALL( DesktopAppUninit( errors ) );
  }

  void DesktopApp::Update( Errors& )
  {
  }

  void DesktopApp::DebugImGui( Errors& errors )
  {
    if( ImGuiCollapsingHeader( "DesktopApp::DebugImGui" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      Platform::PlatformImGui( errors );
    }
  }

  auto DesktopApp::GetMainErrors() -> Errors& { return gMainFunctionErrors; }

} // namespace Tac


