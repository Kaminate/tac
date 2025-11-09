#pragma once

#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

namespace Tac
{
  struct CreationAppState : public App::IState
  {
    World  mWorld  {};
    Camera mCamera {};
  };

  struct LevelEditorApp : public App
  {
    LevelEditorApp( const Config& );

    void Init( Errors& ) override;
    auto GameState_Create() -> State override;
    void GameState_Update( IState* ) override;
    void Update( Errors& ) override;
    void Render( App::RenderParams, Errors& ) override;
    void Uninit( Errors& ) override;
  };

} // namespace Tac

