#include "tac_examples.h"

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/presentation/game/tac_game_presentation.h"
#include "tac-ecs/presentation/shadow/tac_shadow_presentation.h"
#include "tac-ecs/presentation/skybox/tac_skybox_presentation.h"
#include "tac-ecs/presentation/voxel/tac_voxel_gi_presentation.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-examples/tac_examples_registry.h"
#include "tac-examples/tac_examples_state_machine.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{

  struct ExampleState : public App::IState
  {
    World  mWorld  {};
    Camera mCamera {};
  };

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
  static Debug3DDrawBuffers sDebug3DDrawBuffers;

  static void   ExamplesInitCallback( Errors& errors )
  {
    // nav
    //const int x { 50 };
    //const int y { 50 };
    //const int w { 400 };
    //const int h { 200 };
    //const int spacing { 50 };

    // demo
    //const int size { 600 };

    //sNavWindow = CreateTrackedWindow( "Example.Nav", x, y, w, h );
    //sDemoWindow = CreateTrackedWindow( "Example.Demo", x + w + spacing, y, size, size  );
    //QuitProgramOnWindowClose( sNavWindow );

    ExampleRegistryPopulate();

    SettingsNode childNode{ sSettingsNode.GetChild( "Example.Name" ) };
    const String settingExampleName { childNode.GetValueWithFallback( "" ) };
    const int    settingExampleIndex { GetExampleIndex( settingExampleName ) };

    SetNextExample( settingExampleIndex );

    TAC_CALL( GamePresentation::Init( errors ));
  }

  static void   ExamplesUninitCallback( Errors& )
  {
    ExampleStateMachineUnint();
  }


  static void ExampleSelectorWindow( Errors& )
  {
    ImGuiSetNextWindowStretch();
    static bool showWindow { true };
    if( !showWindow )
      return;

    //ImGuiSetNextWindowHandle( sNavWindow );
    if( !ImGuiBegin( "Examples Selector" ) )
      return;

    TAC_ON_DESTRUCT( ImGuiEnd() );

    if( ImGuiButton( "Close Window" ) )
      showWindow = false;

    sNavWindow = ImGuiGetWindowHandle();


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

    if( offset )
      SetNextExample( iCurrent + offset );

    if( iSelected != -1 )
      SetNextExample( iSelected );
  }

  static void ExampleDemoWindow( Errors& errors )
  {
    ImGuiSetNextWindowStretch();
    ImGuiSetNextWindowDisableBG();
    //ImGuiSetNextWindowHandle( sDemoWindow );
    if( !ImGuiBegin( "Examples Demo" ) )
      return;

    sDemoWindow = ImGuiGetWindowHandle();
    TAC_ON_DESTRUCT( ImGuiEnd() );
    const int iOld{ GetCurrExampleIndex() };
    TAC_CALL( ExampleStateMachineUpdate( errors ) );
    const int iNew { GetCurrExampleIndex() };
    if( iOld != iNew )
    {
      sSettingsNode.GetChild( "Example.Name" ).SetValue( GetExampleName( iNew ) );
    }
  }

  static void ExamplesRenderCallback( App::RenderParams appRenderParams, Errors& errors )
  {

    if( !sDemoWindow.IsValid() || !AppWindowApi::IsShown( sDemoWindow ) )
      return;

    const v2i windowSize{ AppWindowApi::GetSize( sDemoWindow ) };
    ExampleState* state{ ( ExampleState* )appRenderParams.mNewState };
    if( !state )
      return;

    const Render::SwapChainHandle swapChain{ AppWindowApi::GetSwapChainHandle( sDemoWindow ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::TextureHandle backbufferColor{
      renderDevice->GetSwapChainCurrentColor( swapChain ) };

    const Render::TextureHandle backbufferDepth{
      renderDevice->GetSwapChainDepth( swapChain ) };

    TAC_CALL( Render::IContext::Scope renderContextScope{
      renderDevice->CreateRenderContext( errors ) } );

    Render::IContext* renderContext{renderContextScope.GetContext()};

    renderContext->ClearColor( backbufferColor, v4( 0, 0, 0, 1 ) );
    renderContext->ClearDepth( backbufferDepth, 1.0f );

    const GamePresentation::RenderParams gamePresentationRenderParams
    {
      .mContext            { renderContext },
      .mWorld              { &state->mWorld },
      .mCamera             { &state->mCamera },
      .mViewSize           { windowSize },
      .mColor              { backbufferColor },
      .mDepth              { backbufferDepth },
      .mBuffers            { &sDebug3DDrawBuffers },
    };
    TAC_CALL( GamePresentation::Render( gamePresentationRenderParams, errors ) );
    TAC_CALL( renderContext->Execute( errors ) );
  }

  static void ExamplesUpdateCallback( Errors& errors )
  {
    if( AppKeyboardApi::IsPressed( Key::Escape ) )
      OS::OSAppStopRunning();

    TAC_CALL( ExampleDemoWindow( errors ) );
    TAC_CALL( ExampleSelectorWindow( errors ) );
  }


  struct ExamplesApp : public App
  {
    ExamplesApp( const Config& config ) : App( config ) {}
    void Init( Errors& errors ) override
    {
      sSettingsNode = mSettingsNode;
      SpaceInit();
      TAC_CALL( SkyboxPresentation::Init( errors ) );
      TAC_CALL( GamePresentation::Init( errors ) );
      TAC_CALL( ShadowPresentation::Init( errors ) );
      //TAC_CALL( VoxelGIPresentationInit( errors ) );
      ExamplesInitCallback( errors );
    }

    void Update( Errors& errors ) override
    {
      ExamplesUpdateCallback( errors );
    }

    void Render( RenderParams renderParams, Errors& errors ) override
    {
      ExamplesRenderCallback( renderParams, errors );
    }

    void Present( Errors& ) override
    {
      //const SysWindowApi windowApi{ presentParams.mWindowApi };
      //const WindowHandle handles[]{ sNavWindow, sDemoWindow };
      //for( WindowHandle handle : handles )
      //{
      //  if( AppWindowApi::IsShown( handle ) )
      //  {
      //    const Render::SwapChainHandle swapChain{ AppWindowApi::GetSwapChainHandle( handle ) };
      //    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      //    TAC_CALL( renderDevice->Present( swapChain, errors ) );
      //  }
      //}
    }

    State GameState_Create() override
    {
      return TAC_NEW ExampleState;
    }

    void GameState_Update( IState* state ) override
    {
      if( Example * ex{ GetCurrExample() } )
      {
        ( ( ExampleState* )state )->mCamera = *ex->mCamera;
        ( ( ExampleState* )state )->mWorld.DeepCopy( *ex->mWorld );
      }
    }

    void Uninit( Errors& errors ) override { ExamplesUninitCallback( errors ); }
  };

  App* App::Create()
  {
    const App::Config cfg{ .mName { "Examples" } };
    return TAC_NEW ExamplesApp(cfg);
  }

  v3 Example::GetWorldspaceKeyboardDir()
  {
    v3 force{};
    force += AppKeyboardApi::IsPressed( Key::W ) ? mCamera->mUp : v3{};
    force += AppKeyboardApi::IsPressed( Key::A ) ? -mCamera->mRight : v3{};
    force += AppKeyboardApi::IsPressed( Key::S ) ? -mCamera->mUp : v3{};
    force += AppKeyboardApi::IsPressed( Key::D ) ? mCamera->mRight : v3{};
    if( const float q{ force.Quadrance() }; q )
      force /= Sqrt( q );

    return force;
  }

  AssetPathString Example::GetFileAssetPath( const char* filename )
  {
    AssetPathString result;
    result += "assets/";
    result += App::Instance()->GetAppName();
    result += "/";
    result += filename;
    return result;
  }

} // namespace Tac
