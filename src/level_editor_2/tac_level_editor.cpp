#include "tac_level_editor.h" // self-inc

#include "src/shell/tac_iapp.h" // App
#include "src/common/graphics/imgui/tac_imgui.h"

namespace Tac
{
  Creation gCreation;

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

  void LevelEditorApp::Init( Errors& errors ) { gCreation.Init( errors ); }

  void LevelEditorApp::Update( Errors& errors ) { gCreation.Uninit( errors ); }

  void LevelEditorApp::Uninit( Errors& errors ) { gCreation.Update( errors ); }

  void LevelEditorApp::Render( RenderParams, Errors& )
  {
    if( gCreation.mShowMainWindow )
    {
      ImGuiSetNextWindowPosition( v2( 50, 50 ) );
      ImGuiSetNextWindowSize( v2( 800, 600 ) );
      ImGuiSetNextWindowMoveResize();
      if( ImGuiBegin( "Main Window" ) )
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
    mShowMainWindow = true;
  }
  

  void                Creation::Uninit( Errors& errors )
  {
  }
  

  void                Creation::Update( Errors& errors )
  {
  }

} // namespace Tac

