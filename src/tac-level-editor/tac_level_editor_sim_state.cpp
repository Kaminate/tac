#include "tac_level_editor_sim_state.h" // self-inc

namespace Tac
{

  
  void CreationSimState::Init( Errors& errors )
  {
    mWorld = TAC_NEW World;
    mEditorCamera = TAC_NEW Camera
    {
      .mPos       { 0, 1, 5 },
      .mForwards  { 0, 0, -1 },
      .mRight     { 1, 0, 0 },
      .mUp        { 0, 1, 0 }
    };
  }

  void CreationSimState::Uninit()
  {
    TAC_DELETE mWorld;
    TAC_DELETE mEditorCamera;
  }
} // namespace Tac

