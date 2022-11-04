#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/tac_camera.h"
#include "src/common/tac_error_handling.h"
#include "src/common/tac_preprocessor.h"
#include "src/common/tac_keyboard_input.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/game-examples/tac_example_phys_sim_1_force.h"
#include "src/space/model/tac_model.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/Simulation-01-Force

namespace Tac
{

#if 0
  ExampleFactory GetExamplePhysSimForceFactory()
  {
    return []() -> Example* { return TAC_NEW ExamplePhysSimForce; };
  }
  #endif

  ExamplePhysSim1Force::ExamplePhysSim1Force()
  {
    mBall.mPos = {};
    mBall.mRadius = 0.7f;
    mBall.mMass = 10;
    mBall.mVelocity = {};
    mSpringConstant = 100;
  }

  void ExamplePhysSim1Force::Update( Errors& errors )
  {
    v3 springForce = mSpringConstant * -mBall.mPos;
    v3 gravityForce = 9.8f * -mCamera->mUp * mBall.mMass;
    v3 dragForce = 0.2f * mBall.mMass * mBall.mVelocity.Length() * -mBall.mVelocity;

    v3 keyboardForce{};
    keyboardForce += KeyboardIsKeyDown( Key::W ) ? mCamera->mUp : v3{};
    keyboardForce += KeyboardIsKeyDown( Key::A ) ? -mCamera->mRight : v3{};
    keyboardForce += KeyboardIsKeyDown( Key::S ) ? -mCamera->mUp : v3{};
    keyboardForce += KeyboardIsKeyDown( Key::D ) ? mCamera->mRight : v3{};
    keyboardForce *= 75.0f / (keyboardForce.Quadrance() ? keyboardForce.Length() : 1.0f);

    v3 totalForce{};
    totalForce += springForce;
    totalForce += gravityForce;
    totalForce += dragForce;
    totalForce += keyboardForce;

    v3 accel = totalForce / mBall.mMass;
    mBall.mVelocity += accel * TAC_DELTA_FRAME_SECONDS;
    mBall.mPos += mBall.mVelocity * TAC_DELTA_FRAME_SECONDS;

    mWorld->mDebug3DDrawData->DebugDraw3DCircle( mBall.mPos, mCamera->mForwards, mBall.mRadius );
    mWorld->mDebug3DDrawData->DebugDraw3DLine(v3(0,0,0), mBall.mPos);
  }

  ExamplePhysSim1Force::~ExamplePhysSim1Force()
  {
  }


} // namespace Tac
