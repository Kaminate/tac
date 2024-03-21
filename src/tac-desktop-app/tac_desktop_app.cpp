#include "tac_desktop_app.h" // self-inc

#include "tac-desktop-app/tac_desktop_app_error_report.h"
#include "tac-desktop-app/tac_desktop_app_renderers.h"
#include "tac-desktop-app/tac_desktop_app_threads.h"
#include "tac-desktop-app/tac_desktop_event.h"
//#include "tac-desktop-app/tac_desktop_window_life.h"
#include "tac-desktop-app/tac_desktop_window_move.h"
#include "tac-desktop-app/tac_desktop_window_resize.h"
#include "tac-desktop-app/tac_desktop_window_settings_tracker.h"
#include "tac-desktop-app/tac_logic_thread.h"
#include "tac-desktop-app/tac_platform_thread.h"
#include "tac-desktop-app/tac_render_state.h"

#include "tac-ecs/tac_space.h"

#include "tac-rhi/render3/tac_render_api.h"

#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/controller/tac_controller_input.h"
//#include "tac-engine-core/hid/tac_keyboard_api.h"
#include "tac-engine-core/net/tac_net.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
//#include "tac-engine-core/window/tac_window_api.h"
//#include "tac-engine-core/window/tac_window_api_graphics.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/hid/tac_keyboard_backend.h"
#include "tac-engine-core/window/tac_window_backend.h"
#include "tac-engine-core/window/tac_sys_window_api.h"

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
  using namespace DesktopEventApi;

  struct DesktopEventHandler : public DesktopEventApi::Handler
  {
    void HandleBegin() override
    {
      mWindowBackend->ApplyBegin();
      mKeyboardBackend->ApplyBegin();
    }

    void HandleEnd() override
    {
      mWindowBackend->ApplyEnd();
      mKeyboardBackend->ApplyEnd();
    }

    void Handle( const WindowDestroyEvent& data ) override
    {
      mWindowBackend->SetWindowDestroyed( data.mWindowHandle );
    }

    void Handle( const WindowCreateEvent& data ) override
    {
      const v2i pos{ data.mX, data.mY };
      const v2i size{ data.mW, data.mH };
      mWindowBackend->SetWindowCreated( data.mWindowHandle,
                                       data.mNativeWindowHandle,
                                       data.mName,
                                       pos,
                                       size );
    }

    void Handle( const CursorUnobscuredEvent& data ) override
    {
      //SetHoveredWindow( data.mWindowHandle );
    }

    void Handle( const KeyInputEvent& data ) override
    {
      mKeyboardBackend->SetCodepoint( data.mCodepoint );
    }

    void Handle( const KeyStateEvent& data ) override
    {
      const KeyboardBackend::KeyState state = data.mDown
        ? KeyboardBackend::KeyState::Down
        : KeyboardBackend::KeyState::Up;

      mKeyboardBackend->SetKeyState( data.mKey, state );
    }

    void Handle( const MouseMoveEvent& data ) override
    {
      const v2 screenSpaceWindowPos = mWindowApi->GetPos( data.mWindowHandle );
      const v2 windowSpaceMousePos{ ( float )data.mX, ( float )data.mY };
      const v2 screenSpaceMousePos = screenSpaceWindowPos + windowSpaceMousePos;
      mKeyboardBackend->SetMousePos(screenSpaceMousePos);
    }

    void Handle( const MouseWheelEvent& data ) override
    {
      mKeyboardBackend->SetMouseWheel( data.mDelta );
    }

    void Handle( const WindowMoveEvent& data ) override
    {
      mWindowBackend->SetWindowPos( data.mWindowHandle, v2i( data.mX, data.mY ) );
    }

    void Handle( const WindowResizeEvent& data ) override
    {
      mWindowBackend->SetWindowSize( data.mWindowHandle, v2i( data.mWidth, data.mHeight ) );
    }

    KeyboardBackend::SysApi* mKeyboardBackend;
    WindowBackend::SysApi* mWindowBackend;
    SysWindowApi* mWindowApi;
  };


  static DesktopEventHandler           sDesktopEventHandler;

  static KeyboardBackend::SysApi       sKeyboardBackend;
  static WindowBackend::SysApi         sWindowBackend;

  static Errors                        gPlatformThreadErrors( Errors::kDebugBreaks );
  static Errors                        gLogicThreadErrors( Errors::kDebugBreaks );
  static Errors                        gMainFunctionErrors( Errors::kDebugBreaks );

  static App*                          sApp;

  static GameStateManager              sGameStateManager;
  static DesktopApp                    sDesktopApp;
  static SimKeyboardApi                sSimKeyboardApi;
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

  // -----------------------------------------------------------------------------------------------

  static String ToStem( StringView sv )
  {
    String result;
    for( char c : sv )
    {
      const bool isValid = IsAlpha( c ) || IsDigit( c ) || c == ' ' || c == '_';
      result += isValid ? c : ' ';
    }

    if( result.empty() )
      return "tac";

    return result;
  }

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
    LogicThread sLogicThread =
    {
      .mApp = sApp,
      .mErrors = &gLogicThreadErrors,
      .sGameStateManager = &sGameStateManager,
    };

    PlatformThread sPlatformThread =
    {
      .mApp = sApp,
      .mErrors = &gLogicThreadErrors,
      .mGameStateManager = &sGameStateManager,
    };

    TAC_CALL( Render::RenderApi::Init( {}, errors ) );

    const ImGuiInitParams imguiInitParams 
    {
      .mMaxGpuFrameCount = Render::RenderApi::GetMaxGPUFrameCount() ,
      //.mSetWindowPos = ImGuiSimSetWindowPos,
      //.mSetWindowSize = ImGuiSimSetWindowSize,
      //.mCreateWindow = ImGuiSimCreateWindow,
      //.mDestroyWindow = ImGuiSimDestroyWindow,
      .mSimWindowApi = &sSimWindowApi,
      .mSimKeyboardApi = &sSimKeyboardApi,
    };
    ImGuiInit( imguiInitParams );

    sDesktopEventHandler.mKeyboardBackend = &sKeyboardBackend;
    sDesktopEventHandler.mWindowBackend = &sWindowBackend;
    sDesktopEventHandler.mWindowApi = &sSysWindowApi;
    DesktopEventApi::Init( &sDesktopEventHandler );

    std::thread logicThread( &LogicThread::Update, sLogicThread, std::ref( gLogicThreadErrors ) );

    sPlatformThread.mApp = sApp;
    sPlatformThread.mErrors = &gPlatformThreadErrors;
    sPlatformThread.Update( gPlatformThreadErrors );
    logicThread.join();

    sPlatformThread.Uninit();
    sLogicThread.Uninit();
    ImGuiUninit();

    DesktopAppErrorReport errorReport;
    errorReport.Add( "Platform Thread", &gPlatformThreadErrors );
    errorReport.Add( "Main Function", &gMainFunctionErrors );
    errorReport.Add( "Logic Thread", &gLogicThreadErrors );
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



  static void         DesktopAppDebugImGuiHoveredWindow()
  {
    PlatformFns* platform = PlatformFns::GetInstance();
#if 0
    const WindowHandle hoveredHandle = platform->PlatformGetMouseHoveredWindow();
    const DesktopWindowState* hovered = hoveredHandle.GetDesktopWindowState();
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
#endif
  }

  void                DesktopApp::DebugImGui(Errors& errors)
  {
    if( !ImGuiCollapsingHeader("DesktopAppDebugImGui"))
      return;

    TAC_IMGUI_INDENT_BLOCK;

    //DesktopWindowDebugImgui();

    DesktopAppDebugImGuiHoveredWindow();

    PlatformFns* platform =  PlatformFns::GetInstance();
    platform->PlatformImGui( errors );
  }

  // -----------------------------------------------------------------------------------------------




} // namespace Tac

Tac::Errors&             Tac::GetMainErrors() { return gMainFunctionErrors; }


