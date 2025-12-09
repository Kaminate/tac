#include "tac_example_phys_sim_1_force.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/renderpass/game/tac_game_presentation.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/Simulation-01-Force

namespace Tac
{
  void ExamplePhysSim1Force::Update( Errors& )
  {
    const v3 keyboardForceDir{ GetWorldspaceKeyboardDir() };
    const v3 springForce { mSpringConstant * -mBall.mPos };
    const v3 gravityForce { 9.8f * -mCamera->mUp * mBall.mMass };
    const v3 dragForce { 0.2f * mBall.mMass * mBall.mVelocity.Length() * -mBall.mVelocity };
    const v3 keyboardForce { keyboardForceDir * 75.0f };
    const v3 totalForce{ springForce + gravityForce + dragForce + keyboardForce };
    const v3 accel { totalForce / mBall.mMass };
    mBall.mVelocity += accel * TAC_DT;
    mBall.mPos += mBall.mVelocity * TAC_DT;
    mWorld->mDebug3DDrawData->DebugDraw3DCircle( mBall.mPos, mCamera->mForwards, mBall.mRadius );
    mWorld->mDebug3DDrawData->DebugDraw3DLine( v3( 0, 0, 0 ), mBall.mPos );
    mWorld->mDebug3DDrawData->DebugDraw3DLine( mBall.mPos, mBall.mPos + keyboardForceDir );
    ImGuiText( "WASD to apply force" );
  }
} // namespace Tac
