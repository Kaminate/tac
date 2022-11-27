#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/math/tac_math.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/tac_camera.h"
#include "src/common/tac_keyboard_input.h"
#include "src/game-examples/tac_example_phys_sim_5_lincollision.h"
#include "src/space/tac_world.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/...

namespace Tac
{
  static m3 InertiaTensorSphere( float mass, float radius )
  {
    const float s = ( 2.0f / 5.0f ) * mass * radius * radius;
    return { s, 0, 0,
             0, s, 0,
             0, 0, s };
  }

  struct CollisionResult
  {
    bool mCollided = false;
    v3 mNormal; // collision normal from obj A to obj B
    v3 mPoint; // collision point
    float mDist; // penetration distance
  };

  CollisionResult Collide(const ExamplePhys5SimObj& objA, const ExamplePhys5SimObj& objB)
  {
    v3 n = objB.mLinPos - objA.mLinPos;
    float q = n.Quadrance();
    if( q > objA.mRadius * objA.mRadius + objB.mRadius * objB.mRadius )
      return {};
    
    n /= Sqrt(q);
  }

  ExamplePhys5SimObj::ExamplePhys5SimObj()
  {
    ComputeInertiaTensor();
  }


  void ExamplePhys5SimObj::ComputeInertiaTensor()
  {
    const m3 inertiaTensor = InertiaTensorSphere( mMass, mRadius );
    const bool inverted = inertiaTensor.Invert( &mAngInvInertiaTensor );
    TAC_ASSERT( inverted );
  }

  void ExamplePhys5SimObj::BeginFrame()
  {
    mLinForceAccum = {};
    mAngTorqueAccum = {};
  }

  void ExamplePhys5SimObj::Integrate()
  {
    const float dt = TAC_DELTA_FRAME_SECONDS;
    const v3 a = mLinForceAccum / mMass;
    mAngVel += a * dt;
    mLinPos += mAngVel * dt;

  }

  void ExamplePhys5SimObj::AddForce( v3 force )
  {
    mLinForceAccum += force;
  }

  ExamplePhysSim5LinCollision::ExamplePhysSim5LinCollision()
  {
    mPlayer.mLinPos = { 2, 1, 0 };
    mPlayer.mElasticity = 0.75f;
    mPlayer.mColor = v3{ 37, 150, 190 } / 255.0f;

    mObstacle.mLinPos = { -5, 0, 0 };
    mObstacle.mElasticity = 0.65f;
    mObstacle.mMass = 20.0f;
    mObstacle.ComputeInertiaTensor();
    mObstacle.mColor = v3{ 224, 49, 92 } / 255.0f;

    mCamera->mPos = { 0, 2, 10 };
  }



  void ExamplePhysSim5LinCollision::Update( Errors& )
  {
    mPlayer.BeginFrame();
    mObstacle.BeginFrame();

    const v3 keyboardForce = GetWorldspaceKeyboardDir() * 50.0f;

    mPlayer.AddForce( keyboardForce );

    mPlayer.Integrate();
    mObstacle.Integrate();


    mPlayer.mAngRot = m3::RotRadZ( (float)ShellGetElapsedSeconds() );

    Draw( mPlayer );
    Draw( mObstacle );
  }

  void ExamplePhysSim5LinCollision::Draw( const ExamplePhys5SimObj& obj )
  {
    Debug3DDrawData* drawData = mWorld->mDebug3DDrawData;
    drawData->DebugDraw3DCircle( obj.mLinPos, mCamera->mForwards, obj.mRadius, obj.mColor );

    v3 line = obj.mAngRot * v3( obj.mRadius, 0, 0 );;
    drawData->DebugDraw3DLine( obj.mLinPos, obj.mLinPos + line, obj.mColor );
  }


} // namespace Tac
