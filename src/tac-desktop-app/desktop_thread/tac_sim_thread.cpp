#include "tac_sim_thread.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app_threads.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h" // App
#include "tac-desktop-app/desktop_app/tac_render_state.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_settings_tracker.h"

#include "tac-ecs/tac_space.h"

#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
//#include "tac-engine-core/graphics/ui/tac_ui_2d.h" // ~UI2DDrawData
#include "tac-engine-core/graphics/ui/imgui/tac_imgui_state.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/hid/controller/tac_controller_input.h"
#include "tac-engine-core/hid/tac_keyboard_backend.h"
#include "tac-engine-core/net/tac_net.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings.h"
#include "tac-engine-core/shell/tac_shell.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/os/tac_os.h"

#include "tac-engine-core/window/tac_window_backend.h"

namespace Tac
{
  KeyboardBackend::SimApi sKeyboardBackendSimApi;
  WindowBackend::SimApi   sWindowBackend;

  void SimThread::Init( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );

    DesktopAppThreads::SetType( DesktopAppThreads::ThreadType::Sim );

    FrameMemoryInitThreadAllocator( 1024 * 1024 * 10  );

    TAC_CALL( ShellInit( errors ) );

#if TAC_FONT_ENABLED()
    TAC_CALL( FontApi::Init( errors ) );
#endif

    TrackWindowInit( sWindowApi );

    SpaceInit();

  }

  void SimThread::Uninit()
  {
    const bool isRenderEnabled = mApp->IsRenderEnabled();
    Errors& errors = *mErrors;
    {
      mApp->Uninit( errors );
      TAC_DELETE mApp;
      mApp = nullptr;
    }


    if( !errors.empty() )
      OS::OSAppStopRunning();

    //if( isRenderEnabled )
    //  Render::SubmitFinish();
  }

  void SimThread::Update( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );
    //PlatformFns* platform = PlatformFns::GetInstance();

    while( OS::OSAppIsRunning() )
    {
      TAC_CALL( DesktopEventApi::Apply( errors ) );
      TAC_CALL( Network::NetApi::Update( errors ) );
      TAC_CALL( SettingsTick( errors ) );

      if( Timestep::Update() )
      {

        // update at the end so that frameindex 0 has timestep 0

        TAC_PROFILE_BLOCK;
        ProfileSetGameFrame();


        // imo, the best time to pump the message queue would be right before simulation update
        // because it reduces input-->sim latency.
        // (ignore input-->render latency because of interp?)
        // So maybe wndproc should be moved here from the platform thread, and Render::SubmitFrame
        // and Render::RenderFrame should be rearranged
        TAC_PROFILE_BLOCK_NAMED( "frame" );

        sWindowBackend.Sync();
        sKeyboardBackendSimApi.Sync();

        const BeginFrameData data =
        {
          .mElapsedSeconds = Timestep::GetElapsedTime(),
          .mMouseHoveredWindow = {} // platform->PlatformGetMouseHoveredWindow(),
        };
        ImGuiBeginFrame( data );

        Controller::UpdateJoysticks();

        App::UpdateParams updateParams
        {
          .mWindowApi = sWindowApi,
          .mKeyboardApi = sKeyboardApi,
        };
        TAC_CALL( mApp->Update( updateParams, errors ) );

        TAC_CALL( ImGuiEndFrame( errors ) );

        //KeyboardEndFrame();
        //Mouse::MouseEndFrame();

        App::IState* gameState = mApp->GetGameState();
        if( !gameState )
          gameState = TAC_NEW App::IState;

        gameState->mFrameIndex = Timestep::GetElapsedFrames();
        gameState->mTimestamp = Timestep::GetElapsedTime();
        gameState->mTimepoint = Timestep::GetLastTick();
        gameState->mImGuiDraws = ImGuiGetSimFrameDraws();

        sGameStateManager->Enqueue( gameState );

        //if( mApp->IsRenderEnabled() )
        //  Render::SubmitFrame();
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage


    } // while
  }

} // namespace Tac
