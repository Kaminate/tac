#include "tac_platform_thread.h" // self-inc

#include "tac-desktop-app/tac_desktop_app.h"
#include "tac-desktop-app/tac_desktop_app.h"
#include "tac-desktop-app/tac_desktop_app_threads.h"
#include "tac-desktop-app/tac_desktop_event.h"
#include "tac-desktop-app/tac_iapp.h"
#include "tac-desktop-app/tac_render_state.h"

#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
//#include "tac-engine-core/graphics/ui/tac_ui_2d.h" // ~UI2DDrawData
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
//#include "tac-engine-core/window/tac_window_api_graphics.h"
#include "tac-engine-core/platform/tac_platform.h"
#include "tac-engine-core/window/tac_window_backend.h"
#include "tac-engine-core/shell/tac_shell_timer.h"
#include "tac-engine-core/hid/tac_keyboard_backend.h"

//#include "tac-rhi/render/tac_render.h"
#include "tac-rhi/render3/tac_render_api.h"
//#include "tac-rhi/renderer/tac_renderer.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h" // Clamp
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/containers/tac_array.h"

namespace Tac::DesktopEventApi
{

  struct DesktopEventHandler : public Handler
  {
    void HandleBegin() override
    {
      WindowBackend::ApplyBegin();
      KeyboardBackend::ApplyBegin();
    }

    void HandleEnd() override
    {
      WindowBackend::ApplyEnd();
      KeyboardBackend::ApplyEnd();
    }

    void Handle( const WindowDestroyEvent& data ) override
    {
      WindowBackend::SetWindowDestroyed( data.mWindowHandle );
    }

    void Handle( const WindowCreateEvent& data ) override
    {
      const v2i pos{ data.mX, data.mY };
      const v2i size{ data.mW, data.mH };
      WindowBackend::SetWindowCreated( data.mWindowHandle,
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
      KeyboardBackend::SetCodepoint( data.mCodepoint );
    }

    void Handle( const KeyStateEvent& data ) override
    {
      const KeyboardBackend::KeyState state = data.mDown
        ? KeyboardBackend::KeyState::Down
        : KeyboardBackend::KeyState::Up;

      KeyboardBackend::SetKeyState( data.mKey, state );
    }

    void Handle( const MouseMoveEvent& data ) override
    {
      const v2 screenSpaceWindowPos = data.mWindowHandle.GetPosf();
      const v2 windowSpaceMousePos{ ( float )data.mX, ( float )data.mY };
      KeyboardBackend::SetMousePos( screenSpaceWindowPos + windowSpaceMousePos );
    }

    void Handle( const MouseWheelEvent& data ) override
    {
      KeyboardBackend::SetMouseWheel( data.mDelta );
    }

    void Handle( const WindowMoveEvent& data ) override
    {
      WindowBackend::SetWindowPos( data.mWindowHandle, v2i( data.mX, data.mY ) );
    }

    void Handle( const WindowResizeEvent& data ) override
    {
      WindowBackend::SetWindowSize( data.mWindowHandle, v2i( data.mWidth, data.mHeight ) );
    }
  };

  static DesktopEventHandler sDesktopEventHandler;
}

namespace Tac
{
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
  void PlatformThread::Uninit()
  {
    Errors& errors = *mErrors;
    if( !errors.empty() )
      OS::OSAppStopRunning();

    //if( mApp->IsRenderEnabled() )
    //  Render::RenderFinish();

  }

  void PlatformThread::Init( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );
    DesktopAppThreads::SetType( DesktopAppThreads::ThreadType::Main );

    FrameMemoryInitThreadAllocator( 1024 * 1024 * 10 );

    DesktopEventApi::Init( &DesktopEventApi::sDesktopEventHandler );
    
    TAC_CALL( Render::RenderApi::Init( {}, errors ) );
    //TAC_CALL( Render::Init2( Render::InitParams{}, errors ) );

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

      // Win32FrameBegin polls wndproc
      TAC_CALL( platform->PlatformFrameBegin( errors ) );

      TAC_CALL( desktopApp->Update( errors ) );
      TAC_CALL( platform->PlatformFrameEnd( errors ) );

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
          .mOldState = pair.mOldState, // A
          .mNewState = pair.mNewState, // B
          .mT = t, // inbetween B and (future) C
        };
        TAC_CALL( mApp->Render( params, errors ) );
        //TAC_CALL( ImGuiEndFrame( errors ) );
        ImGuiPlatformRender( &pair.mNewState->mImGuiDraws );
        //Render::FrameEnd();
      }


      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage
    }

  }
} //namespace Tac
