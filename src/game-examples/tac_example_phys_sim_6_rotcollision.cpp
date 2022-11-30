#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/math/tac_math.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/tac_camera.h"
#include "src/common/tac_keyboard_input.h"
#include "src/game-examples/tac_example_phys_sim_6_rotcollision.h"
#include "src/space/tac_world.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/...

namespace Tac
{

  struct Sim6CollisionResult
  {
    bool mCollided = false;
    v3 mNormal; // collision normal from obj A to obj B
    v3 mPoint; // collision point
    float mDist; // penetration distance
  };

  static Sim6CollisionResult Sim6Collide(const ExamplePhys6SimObj& objA, const ExamplePhys6SimObj& objB)
  {
    const v3 dx = objB.mLinPos - objA.mLinPos; // vector from objA to objB
    const float q = dx.Quadrance(); // quadrance between circles
    const float rSum = objA.mRadius + objB.mRadius;
    if( q > rSum * rSum )
      return {};
    
    const float d = Sqrt(q);
    const v3 n = dx / d;
    const float penetrationDist = rSum - d;
    const v3 ptA = objA.mLinPos + objA.mRadius * n;
    const v3 ptB = objB.mLinPos - objB.mRadius * n;
    const v3 pt = ( ptA + ptB ) / 2;
    return {
      .mCollided = true,
      .mNormal = n,
      .mPoint = pt,
      .mDist = penetrationDist
    };
  }

  static void Sim6ResolveCollision( const Sim6CollisionResult& collisionResult,
                                ExamplePhys6SimObj& objA,
                                ExamplePhys6SimObj& objB )
  {
    if( !collisionResult.mCollided)
      return;
    objA.mLinPos -= 0.5f * collisionResult.mDist * collisionResult.mNormal;
    objB.mLinPos += 0.5f * collisionResult.mDist * collisionResult.mNormal;
    const v3 relVel = objA.mLinVel - objB.mLinVel;
    const float vDotn = Dot( relVel, collisionResult.mNormal );
    if( vDotn < 0 )
      return;
    const float restitution = objA.mElasticity * objB.mElasticity;
    const float j = vDotn * ( -restitution - 1 ) / ( 1 / objA.mMass + 1 / objB.mMass );
    objA.mLinVel += ( j / objA.mMass ) * collisionResult.mNormal;
    objB.mLinVel -= ( j / objB.mMass ) * collisionResult.mNormal;
  }

  ExamplePhys6SimObj::ExamplePhys6SimObj()
  {
    ComputeInertiaTensor();
  }

  void ExamplePhys6SimObj::ComputeInertiaTensor()
  {
    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
#if 0
    const m3 inertiaTensor = ;
    const bool inverted = inertiaTensor.Invert( &mAngInvInertiaTensor );
    TAC_ASSERT( inverted );
#endif
  }

  void ExamplePhys6SimObj::BeginFrame()
  {
    mLinForceAccum = {};
    mAngTorqueAccum = {};
  }

  void ExamplePhys6SimObj::Integrate()
  {
    const float dt = TAC_DELTA_FRAME_SECONDS;
    const v3 a = mLinForceAccum / mMass;
    mLinVel += a * dt;
    mLinPos += mLinVel * dt;

  }

  void ExamplePhys6SimObj::AddForce( v3 force )
  {
    mLinForceAccum += force;
  }

  ExamplePhysSim6RotCollision::ExamplePhysSim6RotCollision()
  {

  }

  ExamplePhysSim6RotCollision::~ExamplePhysSim6RotCollision()
  {

  }

  void ExamplePhysSim6RotCollision::Update( Errors& )
  {
    mPlayer.BeginFrame();
    mObstacle.BeginFrame();

    const v3 keyboardForce = GetWorldspaceKeyboardDir() * 50.0f;

    mPlayer.AddForce( keyboardForce );

    mPlayer.Integrate();
    mObstacle.Integrate();


    const Sim6CollisionResult collisionResult = Sim6Collide( mPlayer, mObstacle );
    Sim6ResolveCollision( collisionResult, mPlayer, mObstacle );

#if 0
    Player.mAngRot = m3::RotRadZ( (float)ShellGetElapsedSeconds() );
#endif

    Draw( mPlayer );
    Draw( mObstacle );
  }

  void ExamplePhysSim6RotCollision::Draw( const ExamplePhys6SimObj& obj )
  {
    Debug3DDrawData* drawData = mWorld->mDebug3DDrawData;
    drawData->DebugDraw3DCircle( obj.mLinPos, mCamera->mForwards, obj.mRadius, obj.mColor );
    drawData->DebugDraw3DCapsule( p0, p1, radius, obj.mColor );

#if 0
    v3 line = obj.mAngRot * v3( obj.mRadius, 0, 0 );;
    drawData->DebugDraw3DLine( obj.mLinPos, obj.mLinPos + line, obj.mColor );
#endif
  }


} // namespace Tac
