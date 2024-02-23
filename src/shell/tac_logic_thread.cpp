#include "tac_logic_thread.h" // self-inc

#include "src/shell/tac_desktop_app_threads.h"
#include "src/shell/tac_iapp.h"
#include "src/shell/tac_platform.h"
#include "src/shell/tac_desktop_event.h"
#include "src/shell/tac_render_state.h"
#include "src/space/tac_space.h"

#include "src/common/memory/tac_frame_memory.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/system/tac_os.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/graphics/tac_font.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/input/tac_controller_input.h"
#include "src/common/net/tac_net.h"


namespace Tac
{
  static GameStateManager              sGameStateManager;

  void LogicThread::Init( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );

    DesktopAppThreads::SetType(DesktopAppThreads::ThreadType::Logic );

    FrameMemoryInitThreadAllocator(  1024 * 1024 * 10  );

    TAC_CALL( ShellInit( errors ) );

    TAC_CALL( FontApi::Init( errors ) );

    ImGuiInit();
    SpaceInit();

    TAC_CALL( mApp->Init( errors ) );
  }

  void LogicThread::Uninit()
  {
    const bool isRenderEnabled = mApp->IsRenderEnabled();
    Errors& errors = *mErrors;
    {
      mApp->Uninit( errors );
      TAC_DELETE mApp;
      mApp = nullptr;
    }

    ImGuiUninit();

    if( !errors.empty() )
      OS::OSAppStopRunning();

    if( isRenderEnabled )
      Render::SubmitFinish();
  }

  void LogicThread::Update( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );
    PlatformFns* platform = PlatformFns::GetInstance();

    TAC_CALL( Init( errors ) );
    while( OS::OSAppIsRunning() )
    {
      ShellTimerUpdate();
      if( !ShellTimerFrame() )
        continue;

      TAC_PROFILE_BLOCK;
      ProfileSetGameFrame();

      TAC_CALL( SettingsTick( errors ) );
      TAC_CALL( Network::NetApi::Update( errors ) );

      // imo, the best time to pump the message queue would be right before simulation update
      // because it reduces input-->sim latency.
      // (ignore input-->render latency because of interp?)
      // So maybe wndproc should be moved here from the platform thread, and Render::SubmitFrame
      // and Render::RenderFrame should be rearranged
      TAC_PROFILE_BLOCK_NAMED( "frame" );
      DesktopEventApplyQueue();

      Keyboard::KeyboardBeginFrame();
      Mouse::MouseBeginFrame();

      const BeginFrameData data =
      {
        .mElapsedSeconds = ShellGetElapsedSeconds(),
        .mMouseHoveredWindow = platform->PlatformGetMouseHoveredWindow(),
      };
      ImGuiBeginFrame( data );

      Controller::UpdateJoysticks();

      TAC_CALL( mApp->Update( errors ) );

      TAC_CALL( ImGuiEndFrame( errors ) );

      Keyboard::KeyboardEndFrame();
      Mouse::MouseEndFrame();

      ShellIncrementFrameCounter();

      {
        App::IState* gameState = mApp->GetGameState();
        sGameStateManager.Enqueue( gameState );
      }

      if( GameStateManager::Pair gameStatePair = sGameStateManager.Dequeue() )
      {
        const App::RenderParams params
        {
          .mOldState = gameStatePair.mOldState,
          .mNewState = gameStatePair.mNewState,
          .mT = ShellGetInterpolationPercent(),
        };
        TAC_CALL( mApp->Render( params, errors ) );
      }

      if( mApp->IsRenderEnabled() )
        Render::SubmitFrame();

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage

    } // while
  } // LogicThread::Update

} // namespace Tac
