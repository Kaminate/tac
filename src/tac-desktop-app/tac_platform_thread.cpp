#include "tac_platform_thread.h" // self-inc

#include "tac-engine-core/system/tac_desktop_window_graphics.h"
#include "tac-desktop-app/tac_desktop_app.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h" // Clamp
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-rhi/render/tac_render.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-desktop-app/tac_render_state.h"
#include "tac-desktop-app/tac_desktop_app.h"
#include "tac-desktop-app/tac_desktop_app_threads.h"
#include "tac-desktop-app/tac_desktop_event.h"
#include "tac-engine-core/system/tac_platform.h"
#include "tac-desktop-app/tac_iapp.h"

namespace Tac::DesktopEventApi
{
  static struct : public Handler
  {
    void Handle( const AssignHandleEvent& data ) override
    {
      DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
      if( desktopWindowState->mNativeWindowHandle != data.mNativeWindowHandle )
      {
        WindowGraphics::NativeHandleChangedData handleChangedData
        {
          .mDesktopWindowHandle = data.mDesktopWindowHandle,
          .mNativeWindowHandle = data.mNativeWindowHandle,
          .mName = data.mName,
          .mW = data.mW,
          .mH = data.mH,
        };
        WindowGraphics::Instance().NativeHandleChanged( handleChangedData );
      }
      desktopWindowState->mNativeWindowHandle = data.mNativeWindowHandle;
      desktopWindowState->mName = ( StringView )data.mName;
      desktopWindowState->mWidth = data.mW;
      desktopWindowState->mHeight = data.mH;
      desktopWindowState->mX = data.mX;
      desktopWindowState->mY = data.mY;
    }

    void Handle( const CursorUnobscuredEvent& data ) override
    {
      SetHoveredWindow( data.mDesktopWindowHandle );
    }

    void Handle( const KeyInputEvent& data ) override
    {
      Keyboard::KeyboardSetWMCharPressedHax( data.mCodepoint );
    }

    void Handle( const KeyStateEvent& data ) override
    {
      Keyboard::KeyboardSetIsKeyDown( data.mKey, data.mDown );
    }

    void Handle( const MouseButtonStateEvent& data ) override
    {
      Mouse::ButtonSetIsDown( data.mButton, data.mDown );
    }

    void Handle( const MouseMoveEvent& data ) override
    {
      const DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
      const v2 windowPos = desktopWindowState->GetPosV2();
      const v2 dataPos( ( float )data.mX, ( float )data.mY );
      const v2 pos = windowPos + dataPos;
      Mouse::SetScreenspaceCursorPos( pos );
    }

    void Handle( const MouseWheelEvent& data ) override
    {
      Mouse::MouseWheelEvent( data.mDelta );
    }

    void Handle( const WindowMoveEvent& data ) override
    {
      DesktopWindowState* state = GetDesktopWindowState( data.mDesktopWindowHandle );
      state->mX = data.mX;
      state->mY = data.mY;
    }

    void Handle( const WindowResizeEvent& data ) override
    {
      DesktopWindowState* desktopWindowState = GetDesktopWindowState( data.mDesktopWindowHandle );
      desktopWindowState->mWidth = data.mWidth;
      desktopWindowState->mHeight = data.mHeight;
      WindowGraphics::Instance().Resize( data.mDesktopWindowHandle,
                                         desktopWindowState->mWidth,
                                         desktopWindowState->mHeight );
    }
  } sDesktopEventHandler;
}

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

    DesktopEventApi::Init( &DesktopEventApi::sDesktopEventHandler );
    
    TAC_CALL( Render::Init2( Render::InitParams{}, errors ) );

    const ImGuiInitParams imguiInitParams 
    {
      .mMaxGpuFrameCount = Render::GetMaxGPUFrameCount() ,
      .mSetWindowPos = []( DesktopWindowHandle handle, v2 pos )
      {
        PlatformFns* platform = PlatformFns::GetInstance();
        platform->PlatformSetWindowPos( handle, ( int )pos.x, ( int )pos.y );
      },
      .mSetWindowSize = []( DesktopWindowHandle handle, v2 size )
      {
        PlatformFns* platform = PlatformFns::GetInstance();
        platform->PlatformSetWindowSize( handle, ( int )size.x, ( int )size.y );
      },
      .mCreateWindow = []( const ImGuiCreateWindowParams& imguiParams )
      {
        DesktopApp* desktopApp =  DesktopApp::GetInstance();

        DesktopAppCreateWindowParams desktopParams
        {
          .mName = "<unnamed>",
          .mX = (int)imguiParams.mPos.x,
          .mY = (int)imguiParams.mPos.y,
          .mWidth = (int)imguiParams.mSize.x,
          .mHeight = (int)imguiParams.mSize.y,
        };

        return desktopApp->CreateWindow( desktopParams );
      },
      .mDestroyWindow = []( const DesktopWindowHandle& handle)
      {
        DesktopApp* desktopApp =  DesktopApp::GetInstance();

        desktopApp->DestroyWindow( handle );
      },
    };
    ImGuiInit( imguiInitParams );
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
