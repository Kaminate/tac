#include "tac_platform_thread.h" // self-inc

#include "src/common/error/tac_error_handling.h"
#include "src/common/math/tac_math.h" // Clamp
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/graphics/ui/imgui/tac_imgui.h"
#include "src/common/graphics/render/tac_render.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/system/tac_os.h"
#include "src/common/graphics/renderer/tac_renderer.h"
#include "src/common/shell/tac_shell_timestep.h"

#include "src/shell/tac_render_state.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_app_threads.h"
#include "src/shell/tac_desktop_event.h"
#include "src/shell/tac_platform.h"
#include "src/shell/tac_iapp.h"

namespace Tac
{
  static bool sVerbose;
  void PlatformThread::Uninit()
  {
    Errors& errors = *mErrors;
    if( !errors.empty() )
      OS::OSAppStopRunning();

    ImGuiUninit();
    if( mApp->IsRenderEnabled() )
      Render::RenderFinish();

  }

  void PlatformThread::Init( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );
    DesktopAppThreads::SetType( DesktopAppThreads::ThreadType::Main );

    FrameMemoryInitThreadAllocator( 1024 * 1024 * 10 );

    DesktopEventInit();
    
    TAC_CALL( Render::Init2( Render::InitParams{}, errors ) );
    ImGuiInit( Render::GetMaxGPUFrameCount() );
  }

  void PlatformThread::Update( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );
    TAC_CALL( Init( errors ) );

    PlatformFns* platform = PlatformFns::GetInstance();
    DesktopApp* desktopApp = DesktopApp::GetInstance();

    while( OS::OSAppIsRunning() )
    {
      TAC_PROFILE_BLOCK;

      TAC_CALL( platform->PlatformFrameBegin( errors ) );
      TAC_CALL( desktopApp->Update( errors ) );
      TAC_CALL( platform->PlatformFrameEnd( errors ) );

      if( mApp->IsRenderEnabled() )
      {
        TAC_CALL( Render::RenderFrame( errors ) );
      }

      // Interpolate between game states and render
      //
      // Explanation:
      //   Suppose we have game states A, B, and C, except C doesn't exist yet, because C is in
      //   the future. If the current time is 25% of the way from B to C, we render (25% A + 75% B).
      //   This introduces some latency at the expense of misprediction (the alternative is 
      //   predicting 125% B)
      if( GameStateManager::Pair pair = sGameStateManager->Dequeue(); pair.IsValid() )
      {
        const TimestampDifference dt = pair.mNewState->mTimestamp - pair.mOldState->mTimestamp;
        TAC_ASSERT( dt.mSeconds != 0 );

        const Timepoint prevTime = pair.mNewState->mTimepoint;
        const Timepoint currTime = Timestep::GetLastTick();

        float t = ( currTime - prevTime ) / dt;

        // if currTime is inbetween pair.mOldState->mTimestamp and pair.mNewState->mTimestamp,
        // then we should instead of picking the two most recent simulation states, should pick
        // a pair thats one frame less recent
        TAC_ASSERT( t >= 0 );

        if( sVerbose )
          OS::OSDebugPrintLine( String() + "unminned t: " + ToString( t ) );

        t = Min( t, 1.0f );

        const Timestamp interpolatedTimestamp = Lerp( pair.mOldState->mTimestamp.mSeconds,
                                                      pair.mNewState->mTimestamp.mSeconds,
                                                      t );


        const BeginFrameData imguiBeginFrameData =
        {
          .mElapsedSeconds = interpolatedTimestamp,
          .mMouseHoveredWindow = platform->PlatformGetMouseHoveredWindow(),
        };
        ImGuiBeginFrame( imguiBeginFrameData );

        const App::RenderParams params
        {
          .mOldState = pair.mOldState, // A
          .mNewState = pair.mNewState, // B
          .mT = t, // inbetween B and (future) C
        };
        TAC_CALL( mApp->Render( params, errors ) );
        TAC_CALL( ImGuiEndFrame( errors ) );
        //Render::FrameEnd();
      }


      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage
    }

  }
} //namespace Tac
