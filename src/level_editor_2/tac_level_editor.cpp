#include "tac_level_editor.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_iapp.h" // App
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/window/tac_sys_window_api.h"

//#include "tac-rhi/render/tac_render.h"
//#include "tac-rhi/render/tac_render_handles.h"

namespace Tac
{
  Creation     sCreation;
  WindowHandle sWindowHandle;

  // -----------------------------------------------------------------------------------------------

  struct LevelEditorState : public App::IState
  {
  };

  struct LevelEditorApp : public App
  {
    LevelEditorApp( const Config& );
    void Init( InitParams, Errors& ) override;
    void Update( UpdateParams, Errors& ) override;
    void Uninit( Errors& ) override;
    void Render( RenderParams, Errors& ) override;
    IState* GetGameState() override;
  };

  LevelEditorApp::LevelEditorApp( const Config& cfg ) : App( cfg ) {}

  void LevelEditorApp::Init( InitParams initParams, Errors& errors )
  {
    const SysWindowApi* windowApi { initParams.mWindowApi };

    const WindowCreateParams windowCreateParams
    {
      .mName { "level editor" },
      .mPos  { 50, 50 },
      .mSize { 800, 600 }, 
    };

    sWindowHandle = windowApi->CreateWindow( windowCreateParams, errors );
    sCreation.Init( errors );

    {
      //Render::ViewHandle2 viewHandle = Render::CreateView2();
    }
    ++asdf;

  }

  void LevelEditorApp::Update( UpdateParams, Errors& errors )
  {
    sCreation.Update( errors );
  }

  void LevelEditorApp::Uninit( Errors& errors )
  {
    sCreation.Uninit( errors );
  }

  void LevelEditorApp::Render( RenderParams, Errors& )
  {

  }

  App::IState* LevelEditorApp::GetGameState() 
  {
    return TAC_NEW LevelEditorState;
  }

  // -----------------------------------------------------------------------------------------------

  App* App::Create()
  {
    const Config config
    {
        .mName { "Level Editor" },
        .mDisableRenderer { true }, // todo
    };

    return TAC_NEW LevelEditorApp( config );
  }

  // -----------------------------------------------------------------------------------------------

  void                Creation::Init( Errors& errors )
  {
  }
  

  void                Creation::Uninit( Errors& errors )
  {
  }
  

  void                Creation::Update( Errors& errors )
  {
    mShowUnownedWindow = true;
    mShowOwnedWindow = false;

    if( mShowUnownedWindow )
    {
      ImGuiSetNextWindowHandle( sWindowHandle );
      if( ImGuiBegin( "Unowned Window" ) )
      {
        ImGuiButton( "" );
        ImGuiEnd();
      }
    }

    if( mShowOwnedWindow )
    {
      ImGuiSetNextWindowPosition( v2( 500, 100 ) );
      ImGuiSetNextWindowSize( v2( 300, 300 ) );
      ImGuiSetNextWindowMoveResize();
      if( ImGuiBegin( "Owned Window" ) )
      {
        ImGuiButton("");
        ImGuiEnd();
      }
    }

  }

} // namespace Tac

