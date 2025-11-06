#include "tac_level_editor_app.h" // self-inc

#include "tac-level-editor/tac_level_editor.h"

namespace Tac
{
  LevelEditorApp::LevelEditorApp( const Config& cfg ) : App( cfg ) {}

  void LevelEditorApp::Init( Errors& errors )
  {
    SpaceInit();
    Creation::gCreation.Init( mSettingsNode, errors );
  }

  auto LevelEditorApp::GameState_Create() -> State
  {
    return TAC_NEW CreationAppState;
  }

  void LevelEditorApp::GameState_Update( IState* state )
  {
    auto renderState{ ( CreationAppState* )state };
    renderState->mWorld.DeepCopy( *Creation::gCreation.mWorld );
    renderState->mEditorCamera = *Creation::gCreation.mEditorCamera;
  }

  void LevelEditorApp::Update( Errors& errors )
  {
    Creation::gCreation.Update( errors );
  }

  void LevelEditorApp::Render( App::RenderParams renderParams, Errors& errors )
  {
    // todo: interpolate between old and new state?
    CreationAppState* renderState{ ( CreationAppState* )renderParams.mNewState };
    Creation::gCreation.Render( &renderState->mWorld, &renderState->mEditorCamera, errors );
  }

  void LevelEditorApp::Uninit( Errors& errors )
  {
    Creation::gCreation.Uninit( errors );
  }


  auto App::Create() -> App* { return TAC_NEW LevelEditorApp( App::Config{ .mName { "Level Editor" } } ); }

} // namespace Tac

