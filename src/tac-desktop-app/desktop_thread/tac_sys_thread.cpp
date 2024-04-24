#include "tac_sys_thread.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app_threads.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h" // App
#include "tac-desktop-app/desktop_app/tac_render_state.h"

#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/hid/tac_keyboard_backend.h"
#include "tac-engine-core/hid/tac_sys_keyboard_api.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_timer.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"

#include "tac-rhi/render3/tac_render_api.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h" // Clamp
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/containers/tac_array.h"


namespace Tac
{
  static WindowBackend::SysApi   sWindowBackendSysApi;
  //static void                ImGuiSimSetWindowPos( WindowHandle handle, v2i pos )
  //{
  //  PlatformFns* platform = PlatformFns::GetInstance();
  //  platform->PlatformSetWindowPos( handle, pos );
  //}

  //static void                ImGuiSimSetWindowSize( WindowHandle handle, v2i size )
  //{
  //  PlatformFns* platform = PlatformFns::GetInstance();
  //  platform->PlatformSetWindowSize( handle, size );
  //}

  static bool sVerbose;

  void SysThread::Uninit()
  {
    Errors& errors = *mErrors;
    if( !errors.empty() )
      OS::OSAppStopRunning();

    //if( mApp->IsRenderEnabled() )
    //  Render::RenderFinish();

  }

  void SysThread::Init( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );

    DesktopAppThreads::SetType( DesktopAppThreads::ThreadType::Sys );

    FrameMemoryInitThreadAllocator( 1024 * 1024 * 10 );

    
    //TAC_CALL( Render::Init2( Render::InitParams{}, errors ) );

  }

  void SysThread::Update( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );

    PlatformFns* platform = PlatformFns::GetInstance();
    DesktopApp* desktopApp = DesktopApp::GetInstance();

    while( OS::OSAppIsRunning() )
    {
      TAC_PROFILE_BLOCK;

      // Win32FrameBegin polls wndproc
      TAC_CALL( platform->PlatformFrameBegin( errors ) );

      TAC_CALL( desktopApp->Update( errors ) );
      TAC_CALL( platform->PlatformFrameEnd( errors ) );

      TAC_CALL( sWindowBackendSysApi.Sync( errors ) );

      //if( mApp->IsRenderEnabled() )
      //{
      //  TAC_CALL( Render::RenderFrame( errors ) );
      //}

      // Interpolate between game states and render
      //
      // Explanation:
      //   Suppose we have game states A, B, and C, except C doesn't exist yet, because C is in
      //   the future. If the current time is 25% of the way from B to C, we render (25% A + 75% B).
      //   This introduces some latency at the expense of misprediction (the alternative is 
      //   predicting 125% B)
      if( GameStateManager::Pair pair = mGameStateManager->Dequeue(); pair.IsValid() )
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
          OS::OSDebugPrintLine( String() + "t before clamping: " + ToString( t ) );

        t = Min( t, 1.0f );

        const Timestamp interpolatedTimestamp = Lerp( pair.mOldState->mTimestamp.mSeconds,
                                                      pair.mNewState->mTimestamp.mSeconds,
                                                      t );


        const App::RenderParams params
        {
          .mWindowApi = mWindowApi,
          .mKeyboardApi = mKeyboardApi,
          .mOldState = pair.mOldState, // A
          .mNewState = pair.mNewState, // B
          .mT = t, // inbetween B and (future) C, but used to lerp A and B
        };
        TAC_CALL( mApp->Render( params, errors ) );

        ImGuiSysDrawParams imguiDrawParams
        {
          .mSimFrameDraws = &pair.mNewState->mImGuiDraws,
          .mWindowApi = mWindowApi,
          .mTimestamp = interpolatedTimestamp,
        };
        TAC_CALL( ImGuiPlatformRender( &imguiDrawParams, errors ) );
        //Render::FrameEnd();
      }


      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage
    }

  }
} //namespace Tac
