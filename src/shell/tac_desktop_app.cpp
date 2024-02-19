#include "src/shell/tac_desktop_app.h" // self-include

#include "space/tac_space.h"

#include "src/common/containers/tac_fixed_vector.h"
#include "src/common/containers/tac_frame_vector.h"
#include "src/common/containers/tac_ring_buffer.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/dataprocess/tac_log.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_font.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/input/tac_controller_input.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/math/tac_math.h" // Max
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/net/tac_net.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/shell/tac_shell_timestep.h"
#include "src/common/string/tac_string.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/system/tac_os.h"

#include "src/shell/tac_desktop_app_renderers.h"
#include "src/shell/tac_desktop_app_error_report.h"
#include "src/shell/tac_desktop_event.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"
#include "src/shell/tac_desktop_window_life.h"
#include "src/shell/tac_desktop_window_move.h"
#include "src/shell/tac_desktop_window_resize.h"
#include "src/shell/tac_render_state.h"
#include "src/shell/tac_platform.h"
#include "src/shell/tac_desktop_app_threads.h"
#include "src/shell/tac_logic_thread.h"
#include "src/shell/tac_platform_thread.h"

import std; // mutex, thread, type_traits


namespace Tac
{
  static Errors                        gPlatformThreadErrors( Errors::kDebugBreaks );
  static Errors                        gLogicThreadErrors( Errors::kDebugBreaks );
  static Errors                        gMainFunctionErrors( Errors::kDebugBreaks );

  static App*                          sApp;

  static DesktopApp                    sDesktopApp;

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

    // for macos standalone_sdl_vk_1_tri, appDataPath =
    //
    //     /Users/n473/Library/Application Support/Sleeping Studio/Vk Ex/
    //
    // for win32 project standalone_win_vk_1_tri, appDataPath =
    //
    //     C:\Users\Nate\AppData\Roaming + /Sleeping Studio + /Whatever bro
    TAC_RAISE_ERROR_IF( !Filesystem::Exists( sShellPrefPath ),
                        String() + "app data path " + sShellPrefPath.u8string() + " doesnt exist" );

    TAC_CALL( SettingsInit( errors ) );

    if( sApp->IsRenderEnabled() )
    {
      TAC_CALL( DesktopInitRendering( errors ) );
    }
  }

  void                DesktopApp::Run( Errors& errors )
  {
    LogicThread sLogicThread =
    {
      .mApp = sApp,
      .mErrors = &gLogicThreadErrors
    };

    PlatformThread sPlatformThread =
    {
      .mApp = sApp,
      .mErrors = &gLogicThreadErrors,
    };

    std::thread logicThread( &LogicThread::Update, sLogicThread, std::ref(gLogicThreadErrors) );

    sPlatformThread.mApp = sApp;
    sPlatformThread.mErrors = &gPlatformThreadErrors;
    sPlatformThread.Update( gPlatformThreadErrors );
    logicThread.join();

    sPlatformThread.Uninit();
    sLogicThread.Uninit();

    DesktopAppErrorReport errorReport;
    errorReport.Add( "Platform Thread", &gPlatformThreadErrors );
    errorReport.Add( "Main Function", &gMainFunctionErrors );
    errorReport.Add( "Logic Thread", &gLogicThreadErrors );
    errorReport.Report();
  }

  void                DesktopApp::Update( Errors& errors )
  {
    TAC_CALL( DesktopAppUpdateWindowRequests( errors ) );
    DesktopAppUpdateMove();
    DesktopAppUpdateResize();
    UpdateTrackedWindows();
  }

  void                DesktopApp::ResizeControls( const DesktopWindowHandle& desktopWindowHandle,
                                                  int edgePx )
  {
    DesktopAppImplResizeControls( desktopWindowHandle, edgePx );
  }

  void                DesktopApp::MoveControls( const DesktopWindowHandle& desktopWindowHandle,
                                                const DesktopWindowRect& rect )
  {
    DesktopAppImplMoveControls( desktopWindowHandle, rect );
  }

  void                DesktopApp::MoveControls( const DesktopWindowHandle& desktopWindowHandle )
  {
    DesktopAppImplMoveControls( desktopWindowHandle );
  }

  DesktopWindowHandle DesktopApp::CreateWindow( const DesktopAppCreateWindowParams& desktopParams )
  {
    return DesktopAppImplCreateWindow( desktopParams );
  }

  void                DesktopApp::DestroyWindow( const DesktopWindowHandle& desktopWindowHandle )
  {
    return DesktopAppImplDestroyWindow(desktopWindowHandle);
  }



  static void         DesktopAppDebugImGuiHoveredWindow()
  {
    PlatformFns* platform = PlatformFns::GetInstance();
    const DesktopWindowHandle hoveredHandle = platform->PlatformGetMouseHoveredWindow();
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

  void                DesktopApp::DebugImGui(Errors& errors)
  {
    if( !ImGuiCollapsingHeader("DesktopAppDebugImGui"))
      return;

    TAC_IMGUI_INDENT_BLOCK;

    DesktopWindowDebugImgui();

    DesktopAppDebugImGuiHoveredWindow();

    PlatformFns* platform =  PlatformFns::GetInstance();
    platform->PlatformImGui(errors);
  }

  // -----------------------------------------------------------------------------------------------




} // namespace Tac

Tac::Errors&             Tac::GetMainErrors() { return gMainFunctionErrors; }


