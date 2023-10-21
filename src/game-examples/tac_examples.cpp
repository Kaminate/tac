#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/common/math/tac_math.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/system/tac_os.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/game-examples/tac_examples.h"
#include "src/game-examples/tac_examples_state_machine.h"
#include "src/game-examples/tac_examples_presentation.h"
#include "src/game-examples/tac_examples_registry.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"

#include <set>

namespace Tac
{

  Example::Example()
  {
    mWorld = TAC_NEW World;
    mCamera = TAC_NEW Camera{ .mPos = { 0, 0, 5 },
                              .mForwards = { 0, 0, -1 },
                              .mRight = { 1, 0, 0 },
                              .mUp = { 0, 1, 0 } };
  }

  Example::~Example()
  {
    TAC_DELETE mWorld;
    TAC_DELETE mCamera;
  }


  static DesktopWindowHandle       sNavWindow;
  static DesktopWindowHandle       sDemoWindow;

  static void   ExamplesInitCallback( Errors& errors )
  {
    // nav
    int x = 50;
    int y = 50;
    int w = 400;
    int h = 200;
    int spacing = 50;

    // demo
    int size = 600;

    sNavWindow = CreateTrackedWindow( "Example.Nav", x, y, w, h );
    sDemoWindow = CreateTrackedWindow( "Example.Demo", x + w + spacing, y, size, size  );
    QuitProgramOnWindowClose( sNavWindow );

    ExampleRegistryPopulate();

    const StringView settingExampleName = SettingsGetString( "Example.Name", "" );
    const int        settingExampleIndex = GetExampleIndex( settingExampleName );

    SetNextExample(settingExampleIndex);

    GamePresentationInit( errors );
    TAC_HANDLE_ERROR( errors );
  }

  static void   ExamplesUninitCallback( Errors& errors )
  {
    ExampleStateMachineUnint();
  }


  static void ExampleSelectorWindow( Errors& errors )
  {
    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowHandle( sNavWindow );
    if( !ImGuiBegin( "Examples" ) )
      return;

    TAC_ON_DESTRUCT( ImGuiEnd());

    int offset = 0;
    int iSelected = -1;
    if( Example* ex = GetCurrExample() )
    {
      ImGuiTextf( "Current Example: %s", ex->mName ) ;
      offset -= ImGuiButton( "Prev" ) ? 1 : 0;
      ImGuiSameLine();
      offset += ImGuiButton( "Next" ) ? 1 : 0;
    }

    if( ImGuiCollapsingHeader( "Select Example" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( int i = 0; i < GetExampleCount(); ++i )
        if( ImGuiSelectable( GetExampleName(i), i == GetCurrExampleIndex() ) )
          iSelected = i;
    }

    if(offset)
      SetNextExample( GetCurrExampleIndex() + offset );

    if(iSelected != -1)
      SetNextExample( iSelected );
  }

  static void ExampleDemoWindow(Errors& errors)
  {
    auto i = ShellGetFrameIndex();
    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowHandle( sDemoWindow );
    if( !ImGuiBegin( "Examples Demo" ) )
      return;
    TAC_ON_DESTRUCT( ImGuiEnd() );

    DesktopWindowState* demoWindowState = GetDesktopWindowState( sDemoWindow );
    int w = demoWindowState->mWidth;
    int h = demoWindowState->mHeight;

    if( Example* ex = GetCurrExample() )
    {

      const Render::ViewHandle view = WindowGraphicsGetView( sDemoWindow );
      const Render::FramebufferHandle fb = WindowGraphicsGetFramebuffer( sDemoWindow );
      Render::SetViewFramebuffer( view, fb );
      Render::SetViewport( view, Render::Viewport( w, h ) );
      Render::SetViewScissorRect( view, Render::ScissorRect( w, h ) );
      GamePresentationRender( ex->mWorld,
                              ex->mCamera,
                              w,
                              h,
                              view );
    }

    const int iOld = GetCurrExampleIndex();
    ExampleStateMachineUpdate( errors );
    TAC_HANDLE_ERROR( errors );
    const int iNew = GetCurrExampleIndex();
    if( iOld != iNew )
      SettingsSetString( "Example.Name", GetExampleName( iNew ) );
  }

  static void   ExamplesUpdateCallback( Errors& errors )
  {
    if( KeyboardIsKeyDown( Key::Escape ) )
      OS::OSAppStopRunning();

    if( !GetDesktopWindowState( sDemoWindow )->mNativeWindowHandle ||
        !GetDesktopWindowState( sNavWindow )->mNativeWindowHandle )
      return;

    ExampleSelectorWindow( errors );
    TAC_HANDLE_ERROR( errors );

    ExampleDemoWindow(errors);
    TAC_HANDLE_ERROR( errors );
  }

  ExecutableStartupInfo          ExecutableStartupInfo::Init()
  {
    return
    {
      .mAppName = "Examples",
      .mProjectInit = ExamplesInitCallback,
      .mProjectUpdate = ExamplesUpdateCallback,
      .mProjectUninit = ExamplesUninitCallback,
    };
  }

  v3 Example::GetWorldspaceKeyboardDir()
  {
    v3 force{};
    force += KeyboardIsKeyDown( Key::W ) ? mCamera->mUp : v3{};
    force += KeyboardIsKeyDown( Key::A ) ? -mCamera->mRight : v3{};
    force += KeyboardIsKeyDown( Key::S ) ? -mCamera->mUp : v3{};
    force += KeyboardIsKeyDown( Key::D ) ? mCamera->mRight : v3{};
    const float q = force.Quadrance();
    if( q )
      force /= Sqrt( q );
    return force;
  }

} // namespace Tac
