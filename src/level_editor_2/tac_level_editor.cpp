#include "tac_level_editor.h" // self-inc

#include "src/shell/tac_iapp.h" // App
#include "src/shell/tac_desktop_app.h"

#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_render.h"

namespace Tac
{
  Creation gCreation;
  DesktopWindowHandle hWnd;

  // -----------------------------------------------------------------------------------------------

  struct LevelEditorState : public App::IState
  {
  };

  struct LevelEditorApp : public App
  {
    LevelEditorApp( const Config& );
    void Init( Errors& ) override;
    void Update( Errors& ) override;
    void Uninit( Errors& ) override;
    void Render( RenderParams, Errors& ) override;
    IState* GetGameState() override;
  };

  LevelEditorApp::LevelEditorApp( const Config& cfg ) : App( cfg ) {}

  void LevelEditorApp::Init( Errors& errors )
  {
    DesktopApp* desktopApp = DesktopApp::GetInstance();

    DesktopAppCreateWindowParams params
    {
      .mName = "level editor",
      .mX = 50,
      .mY = 50,
      .mWidth = 800,
      .mHeight = 600,
    };
    hWnd = desktopApp->CreateWindow(params);
    gCreation.Init( errors );
  }

  void LevelEditorApp::Update( Errors& errors ) { gCreation.Uninit( errors ); }

  void LevelEditorApp::Uninit( Errors& errors ) { gCreation.Update( errors ); }

  void LevelEditorApp::Render( RenderParams, Errors& )
  {
    gCreation.mShowUnownedWindow = true;
    gCreation.mShowOwnedWindow = false;

    if( gCreation.mShowUnownedWindow )
    {
      ImGuiSetNextWindowHandle( hWnd );
      if( ImGuiBegin( "Unowned Window" ) )
      {
        ImGuiButton( "" );
        ImGuiEnd();
      }
    }

    if( gCreation.mShowOwnedWindow )
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

  App::IState* LevelEditorApp::GetGameState() 
  {
    return TAC_NEW LevelEditorState;
  }

  // -----------------------------------------------------------------------------------------------

  App* App::Create()
  {
    const Config config{
        .mName = "Level Editor",
        .mDisableRenderer = true, // todo
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
  }

} // namespace Tac

