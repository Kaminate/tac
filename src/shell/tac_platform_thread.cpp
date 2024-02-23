#include "tac_platform_thread.h" // self-inc

#include "src/common/error/tac_error_handling.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/system/tac_os.h"
#include "src/common/graphics/tac_renderer.h"

#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_app_threads.h"
#include "src/shell/tac_desktop_event.h"
#include "src/shell/tac_platform.h"
#include "src/shell/tac_iapp.h"

namespace Tac
{
  void PlatformThread::Uninit()
  {
    Errors& errors = *mErrors;
    if( !errors.empty() )
      OS::OSAppStopRunning();

    if( mApp->IsRenderEnabled() )
      Render::RenderFinish();
  }

  void PlatformThread::Init( Errors& errors )
  {
    TAC_ASSERT( mErrors && mApp );
    DesktopAppThreads::SetType( DesktopAppThreads::ThreadType::Main );

    FrameMemoryInitThreadAllocator( 1024 * 1024 * 10 );

    DesktopEventInit();
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

      std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); // Dont max out power usage
    }

  }
} //namespace Tac
