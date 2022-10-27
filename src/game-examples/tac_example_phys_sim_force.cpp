#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/tac_camera.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_preprocessor.h"
#include "src/common/tac_keyboard_input.h"
#include "src/game-examples/tac_example_phys_sim_force.h"
#include "src/space/model/tac_model.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/Simulation-01-Force

namespace Tac
{
  void ExamplePhysSimForce::Init( Errors& errors )
  {
    mWorld = TAC_NEW World;
    mCamera = TAC_NEW Camera{ .mPos = { 0, 1, 5 },
                              .mForwards = { 0, 0, -1 },
                              .mRight = { 1, 0, 0 },
                              .mUp = { 0, 1, 0 } };

#if 0
    EntityUUIDCounter u;
    EntityUUID id = u.AllocateNewUUID();
    mEntity = mWorld->SpawnEntity( id );
    mModel = ( Model* )mEntity->AddNewComponent( Model().GetEntry() );
    mModel->mModelPath = "assets/editor/single_triangle.obj";
    mModel->mModelPath = "assets/essential/sphere.obj";
    mModel->mModelIndex = 0;
#endif

    mBall.mPos = {};
    mBall.mRadius = 0.7f;

    TAC_HANDLE_ERROR( errors );
  }

  void ExamplePhysSimForce::Update( Errors& errors )
  {
    v3 force{};
    if( KeyboardIsKeyJustDown( Key::W ) )
    {
    }


    mBall.mForce = {};


    mWorld->mDebug3DDrawData->DebugDraw3DCircle( mBall.mPos, mCamera->mForwards, mBall.mRadius );
  }

  void ExamplePhysSimForce::Uninit( Errors& errors )
  {
    TAC_DELETE mWorld;
    TAC_DELETE mCamera;
  }

  const char* ExamplePhysSimForce::GetName() const { return "Phys Sim Force"; }

} // namespace Tac
