#include "tac_level_editor_app.h" // self-inc

#include "tac-level-editor/tac_level_editor.h"

namespace Tac
{
  LevelEditorApp::LevelEditorApp( const Config& cfg ) : App( cfg ) {}

  void LevelEditorApp::Init( Errors& errors )
  {
    SpaceInit();
    Creation::Init( errors );
  }

  auto LevelEditorApp::GameState_Create() -> State { return TAC_NEW CreationAppState; }

  void LevelEditorApp::GameState_Update( IState* state )
  {
    World* world{ Creation::GetWorld() };
    Camera* camera{ Creation::GetCamera() };
    auto renderState{ ( CreationAppState* )state };
    renderState->mWorld.DeepCopy( *world );
    renderState->mCamera = camera ? *camera : Optional<Camera>{};
  }

  void LevelEditorApp::Update( Errors& errors ) { Creation::Update( errors ); }

  void LevelEditorApp::Render( App::RenderParams renderParams, Errors& errors )
  {
    // todo: interpolate between old and new state?
    CreationAppState* renderState{ ( CreationAppState* )renderParams.mNewState };
    const Camera* camera{ renderState->mCamera.HasValue() ? &renderState->mCamera.GetValue() : nullptr };
    Creation::Render( &renderState->mWorld, camera, errors );
  }

  void LevelEditorApp::Uninit( Errors& errors ) { Creation::Uninit( errors ); }


  auto App::Create() -> App* { return TAC_NEW LevelEditorApp( App::Config{ .mName { "Level Editor" } } ); }

} // namespace Tac

