#include "tac_examples.h"

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-ecs/world/tac_world.h"

#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/os/tac_os.h"

#include "tac-examples/tac_examples_registry.h"
#include "tac-examples/tac_examples_state_machine.h"

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
//#include "tac-desktop-app/desktop_window/tac_desktop_window_graphics.h"
#include "tac-desktop-app/desktop_window/tac_desktop_window_settings_tracker.h"

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


  static WindowHandle sNavWindow;
  static WindowHandle sDemoWindow;

  static void   ExamplesInitCallback( Errors& errors )
  {
    // nav
    int x { 50 };
    int y { 50 };
    int w { 400 };
    int h { 200 };
    int spacing { 50 };

    // demo
    int size { 600 };

    sNavWindow = CreateTrackedWindow( "Example.Nav", x, y, w, h );
    sDemoWindow = CreateTrackedWindow( "Example.Demo", x + w + spacing, y, size, size  );
    QuitProgramOnWindowClose( sNavWindow );

    ExampleRegistryPopulate();

    const StringView settingExampleName { SettingsGetString( "Example.Name", "" ) };
    const int        settingExampleIndex { GetExampleIndex( settingExampleName ) };

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

    int offset { 0 };
    int iSelected { -1 };
    const int iCurrent { GetCurrExampleIndex() };
    const int n { GetExampleCount() };
    if( Example* ex { GetCurrExample()  })
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
      for( int i { 0 }; i < n; ++i )
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

    DesktopWindowState* demoWindowState { GetDesktopWindowState( sDemoWindow ) };
    int w { demoWindowState->mWidth };
    int h { demoWindowState->mHeight };

    if( Example* ex = GetCurrExample() )
    {

      const Render::ViewHandle view { WindowGraphicsGetView( sDemoWindow ) };
      const Render::FramebufferHandle fb { WindowGraphicsGetFramebuffer( sDemoWindow ) };
      Render::SetViewFramebuffer( view, fb );
      Render::SetViewport( view, Render::Viewport( w, h ) );
      Render::SetViewScissorRect( view, Render::ScissorRect( w, h ) );
      GamePresentationRender( ex->mWorld,
                              ex->mCamera,
                              w,
                              h,
                              view );
    }

    const int iOld { GetCurrExampleIndex() };
    TAC_CALL( ExampleStateMachineUpdate( errors ));
    const int iNew { GetCurrExampleIndex() };
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
    force += KeyboardIsKeyDown( Key::W ) ? mCamera->mUp : v3{};
    force += KeyboardIsKeyDown( Key::A ) ? -mCamera->mRight : v3{};
    force += KeyboardIsKeyDown( Key::S ) ? -mCamera->mUp : v3{};
    force += KeyboardIsKeyDown( Key::D ) ? mCamera->mRight : v3{};
    const float q { force.Quadrance() };
    if( q )
      force /= Sqrt( q );
    return force;
  }

} // namespace Tac
