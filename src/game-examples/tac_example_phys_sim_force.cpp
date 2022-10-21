#include "src/common/tac_preprocessor.h"
#include "src/common/tac_error_handling.h"
#include "src/game-examples/tac_example_phys_sim_force.h"
#include "src/space/tac_world.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/common/tac_camera.h"
#include "src/space/model/tac_model.h"
#include "src/space/tac_entity.h"

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

    EntityUUIDCounter u;
    EntityUUID id = u.AllocateNewUUID();
    Entity* entity = mWorld->SpawnEntity( id );
    Model* model = ( Model* )entity->AddNewComponent( Model().GetEntry() );
    model->mModelPath = "assets/editor/single_triangle.obj";
    model->mModelPath = "assets/essential/sphere.obj";
    model->mModelIndex = 0;

    mEntity = entity;
    mModel = model;

    TAC_HANDLE_ERROR( errors );
  }

  void ExamplePhysSimForce::Update( Errors& errors )
  {
    //GamePresentationRender(mWorld, mCamera, viewW, viewH, viewHandle );
  }

  void ExamplePhysSimForce::Uninit( Errors& errors )
  {
    TAC_DELETE mWorld;
    TAC_DELETE mCamera;
  }

  const char* ExamplePhysSimForce::GetName() const { return "Phys Sim Force"; }

} // namespace Tac
