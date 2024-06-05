#include "tac_sys_thread.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app_threads.h"
#include "tac-desktop-app/desktop_event/tac_desktop_event.h"

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
#include "tac-engine-core/graphics/ui/tac_font.h"

#include "tac-rhi/render3/tac_render_api.h"

#include "tac-std-lib/math/tac_math.h" // Clamp
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/containers/tac_array.h"


namespace Tac
{
  static SysWindowApiBackend   sWindowBackendSysApi;
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

  static PlatformMouseCursor ImGuiToPlatformMouseCursor( ImGuiMouseCursor imguiCursor )
  {
    switch( imguiCursor )
    {
    case ImGuiMouseCursor::kNone: return PlatformMouseCursor::kNone;
    case ImGuiMouseCursor::kArrow: return PlatformMouseCursor::kArrow;
    case ImGuiMouseCursor::kResizeNS: return PlatformMouseCursor::kResizeNS;
    case ImGuiMouseCursor::kResizeEW: return PlatformMouseCursor::kResizeEW;
    case ImGuiMouseCursor::kResizeNE_SW: return PlatformMouseCursor::kResizeNE_SW;
    case ImGuiMouseCursor::kResizeNW_SE: return PlatformMouseCursor::kResizeNW_SE;
    default: TAC_ASSERT_INVALID_CASE( imguiCursor ) ; return {};
    }
  }

  void SysThread::Uninit()
  {
    Errors& errors { *mErrors };
    if( !errors.empty() )
      OS::OSAppStopRunning();

    //if( mApp->IsRenderEnabled() )
    //  Render::RenderFinish();

  }

  static ThreadAllocator sSysThreadAllocator;

  void SysThread::Init( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );
    sSysThreadAllocator.Init(  1024 * 1024 * 10  ); // 10 MB
    //TAC_CALL( Render::Init2( Render::InitParams{}, errors ) );
  }

  void SysThread::Update( Errors& errors )
  {
    FrameMemorySetThreadAllocator( & sSysThreadAllocator );
    TAC_ASSERT( mErrors && mApp );

    PlatformFns* platform { PlatformFns::GetInstance() };
    DesktopApp* desktopApp { DesktopApp::GetInstance() };

    while( OS::OSAppIsRunning() )
    {
      TAC_PROFILE_BLOCK;

      // Win32FrameBegin polls wndproc
      TAC_CALL( platform->PlatformFrameBegin( errors ) );

      // Apply queued wndproc to saved keyboard/window state
      TAC_CALL( DesktopEventApi::Apply( errors ) );

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
      if( GameStateManager::Pair pair { mGameStateManager->Dequeue() }; pair.IsValid() )
      {
        const TimestampDifference dt { pair.mNewState->mTimestamp - pair.mOldState->mTimestamp };
        TAC_ASSERT( dt.mSeconds != 0 );

        const Timepoint prevTime { pair.mNewState->mTimepoint };
        const Timepoint currTime { Timepoint::Now() };

        float t { ( currTime - prevTime ) / dt };

        // if currTime is inbetween pair.mOldState->mTimestamp and pair.mNewState->mTimestamp,
        // then we should instead of picking the two most recent simulation states, should pick
        // a pair thats one frame less recent
        TAC_ASSERT( t >= 0 );

        if( sVerbose )
          OS::OSDebugPrintLine( String() + "t before clamping: " + ToString( t ) );

        t = Min( t, 1.0f );

        const Timestamp interpolatedTimestamp{
          Lerp( pair.mOldState->mTimestamp.mSeconds, pair.mNewState->mTimestamp.mSeconds, t ) };

        TAC_CALL( FontApi::UpdateGPU( errors ) );

        const App::RenderParams renderParams
        {
          .mWindowApi   { mWindowApi },
          .mKeyboardApi { mKeyboardApi },
          .mOldState    { pair.mOldState }, // A
          .mNewState    { pair.mNewState }, // B
          .mT           { t }, // inbetween B and (future) C, but used to lerp A and B
          .mTimestamp   { interpolatedTimestamp },
        };
        TAC_CALL( mApp->Render( renderParams, errors ) );

        const ImGuiSysDrawParams imguiDrawParams
        {
          .mSimFrameDraws { &pair.mNewState->mImGuiDraws },
          .mWindowApi     { mWindowApi },
          .mTimestamp     { interpolatedTimestamp },
        };
        TAC_CALL( ImGuiPlatformRender( imguiDrawParams, errors ) );

        static PlatformMouseCursor oldCursor{ PlatformMouseCursor::kNone };
        const PlatformMouseCursor newCursor{
          ImGuiToPlatformMouseCursor( pair.mNewState->mImGuiDraws.mCursor ) };
        if( oldCursor != newCursor )
        {
          oldCursor = newCursor;
          platform->PlatformSetMouseCursor( newCursor );
          OS::OSDebugPrintLine( "set mouse cursor : " + ToString( (int)newCursor ) );
        }

        for( const auto& sizeData : pair.mNewState->mImGuiDraws.mWindowSizeDatas )
        {
          if( sizeData.mRequestedPosition.HasValue() )
          {
            const v2i windowPos{ mWindowApi->GetPos( sizeData.mWindowHandle ) };
            const v2i windowPosRequest{ sizeData.mRequestedPosition.GetValue() };
            if( windowPos != windowPosRequest )
              mWindowApi->SetPos( sizeData.mWindowHandle, windowPosRequest );
          }

          if( sizeData.mRequestedSize.HasValue() )
          {
            const v2i windowSize{ mWindowApi->GetSize( sizeData.mWindowHandle ) };
            const v2i windowSizeRequest{ sizeData.mRequestedSize.GetValue() };
            if( windowSize != windowSizeRequest )
              mWindowApi->SetSize( sizeData.mWindowHandle, windowSizeRequest );
          }
        }

        const App::PresentParams presentParams
        {
          .mWindowApi { mWindowApi },
        };
        TAC_CALL( mApp->Present( presentParams, errors ) );
        TAC_CALL( ImGuiPlatformPresent( mWindowApi, errors ) );
        //Render::FrameEnd();
      }


      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage
    }

  }
} //namespace Tac
