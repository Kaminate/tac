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

  CreationSimState::~CreationSimState()
  {
    Clear();
  }

  void CreationSimState::CopyFrom( CreationSimState& other )
  {
    Clear();
    mWorld = TAC_NEW World;
    mEditorCamera = TAC_NEW Camera;
    mWorld->DeepCopy( *other.mWorld );
    *mEditorCamera = *other.mEditorCamera;
  }

  void CreationSimState::Clear()
  {
    TAC_DELETE mWorld;
    mWorld = nullptr;

    TAC_DELETE mEditorCamera;
    mEditorCamera = nullptr;
  }

  void CreationSimState::Uninit()
  {
    Clear();
  }
} // namespace Tac

