#include "tac_examples.h"

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-ecs/presentation/tac_skybox_presentation.h"
#include "tac-ecs/presentation/tac_shadow_presentation.h"
#include "tac-ecs/presentation/tac_voxel_gi_presentation.h"
#include "tac-ecs/world/tac_world.h"


#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
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
      .mPos      { 0, 0, 5 },
      .mForwards { 0, 0, -1 },
      .mRight    { 1, 0, 0 },
      .mUp       { 0, 1, 0 }
    };
  }

  Example::~Example()
  {
    TAC_DELETE mWorld;
    TAC_DELETE mCamera;
  }


  static WindowHandle sNavWindow;
  static WindowHandle sDemoWindow;
  static SettingsNode sSettingsNode;
  static const SimKeyboardApi* sKeyboardApi;
  static const SimWindowApi* sWindowApi;

  static void   ExamplesInitCallback( App::InitParams initParams, Errors& errors )
  {
    // nav
    const int x { 50 };
    const int y { 50 };
    const int w { 400 };
    const int h { 200 };
    const int spacing { 50 };

    // demo
    const int size { 600 };

    sNavWindow = CreateTrackedWindow( "Example.Nav", x, y, w, h );
    sDemoWindow = CreateTrackedWindow( "Example.Demo", x + w + spacing, y, size, size  );
    QuitProgramOnWindowClose( sNavWindow );

    ExampleRegistryPopulate();

    SettingsNode childNode{ sSettingsNode.GetChild( "Example.Name" ) };
    const String settingExampleName { childNode.GetValueWithFallback( "" ) };
    const int    settingExampleIndex { GetExampleIndex( settingExampleName ) };

    SetNextExample( settingExampleIndex );

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

    int offset {  };
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
      for( int i {  }; i < n; ++i )
        if( ImGuiSelectable( GetExampleName(i), i == iCurrent ) )
          iSelected = i;
    }

    if(offset)
      SetNextExample( iCurrent + offset );

    if(iSelected != -1)
      SetNextExample( iSelected );
  }

  static void ExampleDemoWindow( App::UpdateParams appUpdateParams, Errors& errors )
  {
    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowDisableBG();
    ImGuiSetNextWindowHandle( sDemoWindow );
    if( !ImGuiBegin( "Examples Demo" ) )
      return;

    TAC_ON_DESTRUCT( ImGuiEnd() );

    const int iOld{ GetCurrExampleIndex() };

    const Example::UpdateParams exampleUpdateParams
    {
      .mKeyboardApi{ appUpdateParams.mKeyboardApi },
      .mWindowApi{ appUpdateParams.mWindowApi },
    };

    TAC_CALL( ExampleStateMachineUpdate( exampleUpdateParams, errors ) );
    const int iNew { GetCurrExampleIndex() };
    if( iOld != iNew )
    {
      sSettingsNode.GetChild( "Example.Name" ).SetValue( GetExampleName( iNew ) );
    }
  }

  static void ExamplesRenderCallback( App::RenderParams renderParams, Errors& errors )
  {
    const SysWindowApi* windowApi{ renderParams.mWindowApi };
    const v2i windowSize{ windowApi->GetSize( sDemoWindow ) };
    if( Example* ex = GetCurrExample() )
    {
      const Render::SwapChainHandle swapChain{ windowApi->GetSwapChainHandle( sDemoWindow ) };
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      const Render::TextureHandle backbuffer{
        renderDevice->GetSwapChainCurrentColor( swapChain ) };

      TAC_CALL( GamePresentationRender( ex->mWorld,
                              ex->mCamera,
                              windowSize,
                              backbuffer,
                              errors ) );
    }

  }

  static void   ExamplesUpdateCallback( App::UpdateParams updateParams, Errors& errors )
  {
    sKeyboardApi = updateParams.mKeyboardApi;
    sWindowApi = updateParams.mWindowApi;

    if( sKeyboardApi->IsPressed( Key::Escape ) )
      OS::OSAppStopRunning();

    if( !sWindowApi->IsShown(sDemoWindow ) )
      return;

    if( !sWindowApi->IsShown(sNavWindow ) )
      return;

    TAC_CALL( ExampleSelectorWindow( errors ) );

    TAC_CALL( ExampleDemoWindow( updateParams, errors ) );
  }

  struct ExamplesApp : public App
  {
    ExamplesApp( const Config& config ) : App( config ) {}
    void Init( InitParams initParams, Errors& errors ) override
    {
      sSettingsNode = mSettingsNode;
      SpaceInit();
      TAC_CALL( SkyboxPresentationInit( errors ) );
      TAC_CALL( GamePresentationInit( errors ) );
      TAC_CALL( ShadowPresentationInit( errors ) );
      //TAC_CALL( VoxelGIPresentationInit( errors ) );
      ExamplesInitCallback( initParams, errors );
    }

    void Update( UpdateParams updateParams, Errors& errors ) override
    {
      ExamplesUpdateCallback( updateParams, errors );
    }

    void Render( RenderParams renderParams, Errors& errors ) override
    {
      ExamplesRenderCallback( renderParams, errors );
    }

    void Uninit( Errors& errors ) override { ExamplesUninitCallback( errors ); }
  };

  App* App::Create()
  {
    const App::Config cfg
    {
      .mName { "Examples"  },
    };
    return TAC_NEW ExamplesApp(cfg);
  }

  v3 Example::GetWorldspaceKeyboardDir()
  {
    v3 force{};
    force += sKeyboardApi->IsPressed( Key::W ) ? mCamera->mUp : v3{};
    force += sKeyboardApi->IsPressed( Key::A ) ? -mCamera->mRight : v3{};
    force += sKeyboardApi->IsPressed( Key::S ) ? -mCamera->mUp : v3{};
    force += sKeyboardApi->IsPressed( Key::D ) ? mCamera->mRight : v3{};
    const float q { force.Quadrance() };
    if( q )
      force /= Sqrt( q );

    return force;
  }

} // namespace Tac
