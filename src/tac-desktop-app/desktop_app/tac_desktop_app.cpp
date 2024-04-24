#include "tac_desktop_app.h" // self-inc


#include "tac-desktop-app/desktop_app/tac_desktop_app_error_report.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app_renderers.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app_threads.h"
#include "tac-desktop-app/desktop_app/tac_render_state.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event_handler.h"
#include "tac-desktop-app/desktop_thread/tac_sim_thread.h"
#include "tac-desktop-app/desktop_thread/tac_sys_thread.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_move.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_resize.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_settings_tracker.h"
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
#include "tac-engine-core/settings/tac_settings.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"
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
import std; // mutex, thread, type_traits

namespace Tac
{
  static DesktopEventHandler           sDesktopEventHandler;

  static KeyboardBackend::SysApi       sKeyboardBackend;
  static WindowBackend::SysApi         sWindowBackend;

  static Errors                        sSysErrors( Errors::kDebugBreaks );
  static Errors                        SSimErrors( Errors::kDebugBreaks );
  static Errors                        gMainFunctionErrors( Errors::kDebugBreaks );

  static App*                          sApp;

  static GameStateManager              sGameStateManager;
  static DesktopApp                    sDesktopApp;
  static SimKeyboardApi                sSimKeyboardApi;
  static SysKeyboardApi                sSysKeyboardApi;
  static SimWindowApi                  sSimWindowApi;
  static SysWindowApi                  sSysWindowApi;

  // -----------------------------------------------------------------------------------------------

#if 0
  static WindowHandle ImGuiSimCreateWindow( const ImGuiCreateWindowParams& imguiParams )
  {
    DesktopApp* desktopApp = DesktopApp::GetInstance();

    // todo: ... mixing v2 and v2i bad
    const v2 imPos = imguiParams.mPos;
    const v2 imSize = imguiParams.mSize;
    const v2i simPos = v2i( ( int )imPos.x, ( int )imPos.y );
    const v2i simSize = v2i( ( int )imSize.x, ( int )imSize.y );

    //const WindowApi::CreateParams desktopParams
    const SimWindowApi::CreateParams platformParams
    {
      .mName = "<unnamed>",
      .mPos = simPos,
      .mSize = simSize,
    };
    //return desktopApp->CreateWindow( desktopParams );

    return SimWindowApi::CreateWindow( platformParams );
  }

  static void ImGuiSimDestroyWindow( WindowHandle handle )
  {
    WindowApi::DestroyWindow( handle );
  }
#endif


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


  static String ToStem( StringView sv )
  {
    String result;
    for( char c : sv )
    {
      const bool isValid { IsAlpha( c ) || IsDigit( c ) || c == ' ' || c == '_' };
      result += isValid ? c : ' ';
    }

    if( result.empty() )
      return "tac";

    return result;
  }

  // -----------------------------------------------------------------------------------------------

  DesktopApp*         DesktopApp::GetInstance() { return &sDesktopApp; }

  void                DesktopApp::Init( Errors& errors )
  {
    TAC_ASSERT( PlatformFns::GetInstance() );

    sApp = App::Create();
    TAC_ASSERT( !sApp->mConfig.mName.empty() );

    // right place?
    sShellAppName = sApp->mConfig.mName;
    sShellStudioName = sApp->mConfig.mStudioName;
    sShellPrefPath = TAC_CALL( OS::OSGetApplicationDataPath( errors ) );
    sShellInitialWorkingDir = Filesystem::GetCurrentWorkingDirectory();
    TAC_ASSERT( !sShellAppName.empty() && !sShellPrefPath.empty() );

    // for macos standalone_sdl_vk_1_tri, appDataPath =
    //
    //     /Users/n473/Library/Application Support/Sleeping Studio/Vk Ex/
    //
    // for win32 project standalone_win_vk_1_tri, appDataPath =
    //
    //     C:\Users\Nate\AppData\Roaming + /Sleeping Studio + /Whatever bro
    TAC_RAISE_ERROR_IF( !Filesystem::Exists( sShellPrefPath ),
                        String() + "app data path " + sShellPrefPath.u8string() + " doesnt exist" );

    LogApi::LogSetPath( sShellPrefPath / ( ToStem( sShellAppName ) + ".tac.log" ) );

    TAC_CALL( SettingsInit( sShellPrefPath / ( sShellAppName + "Settings.txt" ), errors ) );

    if( sApp->IsRenderEnabled() )
    {
      TAC_CALL( DesktopInitRendering( errors ) );
    }
  }

  void                DesktopApp::Run( Errors& errors )
  {
    SimThread sSimThread
    {
      .mApp              { sApp },
      .mErrors           { &SSimErrors },
      .sGameStateManager { &sGameStateManager },
      .sWindowApi        { &sSimWindowApi },
      .sKeyboardApi      { &sSimKeyboardApi },
    };

    SysThread sSysThread
    {
      .mApp              { sApp },
      .mErrors           { &sSysErrors },
      .mGameStateManager { &sGameStateManager },
      .mWindowApi        { &sSysWindowApi },
      .mKeyboardApi      { &sSysKeyboardApi },
    };

    const Render::RenderApi::InitParams renderApiInitParams
    {
      .mShaderOutputPath { sShellPrefPath },
    };
    TAC_CALL( Render::RenderApi::Init( renderApiInitParams, errors ) );

    const ImGuiInitParams imguiInitParams
    {
      .mMaxGpuFrameCount { Render::RenderApi::GetMaxGPUFrameCount() },
      //.mSetWindowPos { ImGuiSimSetWindowPos },
      //.mSetWindowSize { ImGuiSimSetWindowSize },
      //.mCreateWindow { ImGuiSimCreateWindow },
      //.mDestroyWindow { ImGuiSimDestroyWindow },
      .mSimWindowApi     { &sSimWindowApi },
      .mSimKeyboardApi   { &sSimKeyboardApi },
    };
    ImGuiInit( imguiInitParams );

    //sWindowBackend.mIsRendererEnabled = sApp->IsRenderEnabled();

    sDesktopEventHandler.mKeyboardBackend = &sKeyboardBackend;
    sDesktopEventHandler.mWindowBackend = &sWindowBackend;
    DesktopEventApi::Init( &sDesktopEventHandler );


    // this is kinda hacky
    DesktopAppThreads::SetType( DesktopAppThreads::ThreadType::Sys );

    TAC_CALL( sSysThread.Init( errors ) );
    TAC_CALL( sSimThread.Init( errors ) );

    App::InitParams initParams
    {
      .mWindowApi   { &sSysWindowApi },
      .mKeyboardApi { &sSysKeyboardApi },
    };
    TAC_CALL( sApp->Init( initParams, errors ) );

    std::thread logicThread( &SimThread::Update, sSimThread, std::ref( SSimErrors ) );

    sSysThread.mApp = sApp;
    sSysThread.mErrors = &sSysErrors;
    sSysThread.Update( sSysErrors );
    logicThread.join();

    sSysThread.Uninit();
    sSimThread.Uninit();
    ImGuiUninit();

    DesktopAppErrorReport errorReport;
    errorReport.Add( "Sys Thread", &sSysErrors );
    errorReport.Add( "Sim Thread", &SSimErrors );
    errorReport.Add( "Main Function", &gMainFunctionErrors );
    errorReport.Report();
  }

  void                DesktopApp::Update( Errors& errors )
  {
    //TAC_CALL( DesktopAppUpdateWindowRequests( errors ) );
    DesktopAppUpdateMove();
    DesktopAppUpdateResize();
    UpdateTrackedWindows();
  }

  //void                DesktopApp::ResizeControls( const WindowHandle& WindowHandle,
  //                                                int edgePx )
  //{
  //  DesktopAppImplResizeControls( WindowHandle, edgePx );
  //}

  //void                DesktopApp::MoveControls( const WindowHandle& WindowHandle,
  //                                              const DesktopWindowRect& rect )
  //{
  //  DesktopAppImplMoveControls( WindowHandle, rect );
  //}

  //void                DesktopApp::MoveControls( const WindowHandle& WindowHandle )
  //{
  //  DesktopAppImplMoveControls( WindowHandle );
  //}

  //WindowHandle DesktopApp::CreateWindow( const WindowApi::CreateParams& desktopParams )
  //{
  //  return DesktopAppImplCreateWindow( desktopParams );
  //}

  //void                DesktopApp::DestroyWindow( const WindowHandle& WindowHandle )
  //{
  //  return DesktopAppImplDestroyWindow(WindowHandle);
  //}

  void                DesktopApp::DebugImGui(Errors& errors)
  {
    if( !ImGuiCollapsingHeader("DesktopAppDebugImGui"))
      return;

    TAC_IMGUI_INDENT_BLOCK;

    //DesktopWindowDebugImgui();

    DesktopAppDebugImGuiHoveredWindow();

    PlatformFns* platform { PlatformFns::GetInstance() };
    platform->PlatformImGui( errors );
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac

Tac::Errors&             Tac::GetMainErrors() { return gMainFunctionErrors; }
