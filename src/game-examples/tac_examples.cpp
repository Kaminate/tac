#include "src/game-examples/tac_examples.h"

#include "space/ecs/tac_entity.h"
#include "space/presentation/tac_game_presentation.h"
#include "space/world/tac_world.h"

#include "src/common/dataprocess/tac_settings.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/math/tac_math.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/shell/tac_shell_timestep.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_os.h"

#include "src/game-examples/tac_examples_registry.h"
#include "src/game-examples/tac_examples_state_machine.h"

#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_graphics.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"

namespace Tac
{

  Example::Example()
  {
    mWorld = TAC_NEW World;
    mCamera = TAC_NEW Camera
    {
      .mPos = { 0, 0, 5 },
      .mForwards = { 0, 0, -1 },
      .mRight = { 1, 0, 0 },
      .mUp = { 0, 1, 0 }
    };
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

    TAC_CALL( GamePresentationInit( errors ));
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
    const int iCurrent = GetCurrExampleIndex();
    const int n = GetExampleCount();
    if( Example* ex = GetCurrExample() )
    {
      ImGuiText( String()
                 + "Current Example ("
                 + ToString( iCurrent + 1 )
                 + "/"
                 + ToString( n )
                 + "): "
                 + ex->mName );
      offset -= ImGuiButton( "Prev" ) ? 1 : 0;
      ImGuiSameLine();
      offset += ImGuiButton( "Next" ) ? 1 : 0;
    }

    if( ImGuiCollapsingHeader( "Select Example", ImGuiNodeFlags_DefaultOpen ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( int i = 0; i < n; ++i )
        if( ImGuiSelectable( GetExampleName(i), i == iCurrent ) )
          iSelected = i;
    }

    if(offset)
      SetNextExample( iCurrent + offset );

    if(iSelected != -1)
      SetNextExample( iSelected );
  }

  static void ExampleDemoWindow(Errors& errors)
  {
    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowDisableBG();
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
    TAC_CALL( ExampleStateMachineUpdate( errors ));
    const int iNew = GetCurrExampleIndex();
    if( iOld != iNew )
      SettingsSetString( "Example.Name", GetExampleName( iNew ) );
  }

  static void   ExamplesUpdateCallback( Errors& errors )
  {
    if( Keyboard::KeyboardIsKeyDown( Keyboard::Key::Escape ) )
      OS::OSAppStopRunning();

    if( !GetDesktopWindowState( sDemoWindow )->mNativeWindowHandle ||
        !GetDesktopWindowState( sNavWindow )->mNativeWindowHandle )
      return;

    TAC_CALL( ExampleSelectorWindow( errors ));

    TAC_CALL( ExampleDemoWindow(errors));
  }

  struct ExamplesApp : public App
  {
    ExamplesApp( const Config& config ) : App( config ) {}
    void Init( Errors& errors ) override { ExamplesInitCallback( errors ); }
    void Update( Errors& errors ) override { ExamplesUpdateCallback( errors ); }
    void Uninit( Errors& errors ) override { ExamplesUninitCallback( errors ); }
  };

  App* App::Create() { return TAC_NEW ExamplesApp(Config { .mName = "Examples" }); }

  v3 Example::GetWorldspaceKeyboardDir()
  {
    v3 force{};
    force += Keyboard::KeyboardIsKeyDown( Keyboard::Key::W ) ? mCamera->mUp : v3{};
    force += Keyboard::KeyboardIsKeyDown( Keyboard::Key::A ) ? -mCamera->mRight : v3{};
    force += Keyboard::KeyboardIsKeyDown( Keyboard::Key::S ) ? -mCamera->mUp : v3{};
    force += Keyboard::KeyboardIsKeyDown( Keyboard::Key::D ) ? mCamera->mRight : v3{};
    const float q = force.Quadrance();
    if( q )
      force /= Sqrt( q );
    return force;
  }

} // namespace Tac
