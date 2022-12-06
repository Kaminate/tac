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


static float density = 2.2f;
static bool sEnableGravity;
static bool spin = true;

namespace Tac
{
  static m3 InertiaTensorSphere( float mass, float radius )
  {
    const float s = ( 2.0f / 5.0f ) * mass * radius * radius;
    return { s, 0, 0,
             0, s, 0,
             0, 0, s };
  }

  struct Sim5CollisionResult
  {
    bool mCollided = false;
    v3 mNormal; // collision normal from obj A to obj B
    v3 mPoint; // collision point
    float mDist; // penetration distance
  };

  static Sim5CollisionResult Sim5CollideSphereSphere(const ExamplePhys5SimObj& objA, const ExamplePhys5SimObj& objB)
  {
    const v3 dx = objB.mLinPos - objA.mLinPos; // vector from objA to objB
    const float q = dx.Quadrance(); // quadrance between circles
    const float rSum = objA.mRadius + objB.mRadius;
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

  static Sim5CollisionResult Sim5Collide(const ExamplePhys5SimObj& objA, const ExamplePhys5SimObj& objB)
  {
    const v3 dx = objB.mLinPos - objA.mLinPos; // vector from objA to objB
    const float q = dx.Quadrance(); // quadrance between circles
    const float rSum = objA.mRadius + objB.mRadius;
    if( q > rSum * rSum )
      return {};
    
    return Sim5CollideSphereSphere(objA, objB);
  }

  static void Sim5ResolveCollision( const Sim5CollisionResult& collisionResult,
                                ExamplePhys5SimObj& objA,
                                ExamplePhys5SimObj& objB )
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
    mLinVel += a * dt;
    mLinPos += mLinVel * dt;
  }

  float ExamplePhys5SimObj::Volume()
  {
    return (4.0f / 3.0f) * 3.14f * mRadius * mRadius * mRadius;
  }

  void ExamplePhys5SimObj::AddForce( v3 force )
  {
    mLinForceAccum += force;
  }

  ExamplePhysSim5LinCollision::ExamplePhysSim5LinCollision()
  {
    v3 pA = { 2, 0, 0 };
    v3 pB = { -5, 0, 0 };
    mPlayer.mLinPos = pB;
    mPlayer.mElasticity = 0.75f;
    mPlayer.mColor = v3{ 37, 150, 190 } / 255.0f;
    mPlayer.mName = "Player";
    mPlayer.Recompute();

    mObstacle.mLinPos = pA;
    mObstacle.mElasticity = 0.65f;
    mObstacle.mColor = v3{ 92, 49, 224 } / 255.0f;
    mObstacle.mRadius = 1;
    mObstacle.mName = "Obstacle";
    mObstacle.Recompute();

    mCamera->mPos = { 0, 2, 10 };
  }


  void ExamplePhys5SimObj::Recompute()
  {
    mMass = density * Volume();
    ComputeInertiaTensor();
  }

  static void SimObjUI(ExamplePhys5SimObj& obj)
  {
    if (!ImGuiCollapsingHeader(obj.mName))
      return;

    bool changed = false;
    changed |= ImGuiDragFloat("radius", &obj.mRadius);
    if (changed)
      obj.Recompute();
  }

  void ExamplePhysSim5LinCollision::Update( Errors& )
  {
    mPlayer.BeginFrame();
    mObstacle.BeginFrame();

    const v3 keyboardForce = GetWorldspaceKeyboardDir() * 150.0f;

    mPlayer.AddForce( keyboardForce );

    v3 grav{};
    ImGuiCheckbox("Enable Gravity", &sEnableGravity);

    SimObjUI(mPlayer);
    SimObjUI(mObstacle);

    if (sEnableGravity)
    {
      float g = 10;
      float r = Distance(mPlayer.mLinPos, mObstacle.mLinPos);
      float f = g * mPlayer.mMass * mObstacle.mMass / (r * r);
      grav = Normalize(mObstacle.mLinPos - mPlayer.mLinPos) * f;
      mPlayer.AddForce(grav);
      mObstacle.AddForce(-grav);
    }


    mPlayer.Integrate();
    mObstacle.Integrate();

    if (sEnableGravity)
      mWorld->mDebug3DDrawData->DebugDraw3DArrow(mPlayer.mLinPos,
                                                mPlayer.mLinPos + grav * TAC_DELTA_FRAME_SECONDS,
                                                mPlayer.mColor);
    mWorld->mDebug3DDrawData->DebugDraw3DArrow(mPlayer.mLinPos,
                                              mPlayer.mLinPos + keyboardForce * TAC_DELTA_FRAME_SECONDS,
                                              mPlayer.mColor);

    const Sim5CollisionResult collisionResult = Sim5Collide( mPlayer, mObstacle );
    Sim5ResolveCollision( collisionResult, mPlayer, mObstacle );

    if(spin)
        mPlayer.mAngRot = m3::RotRadZ( (float)ShellGetElapsedSeconds() );


    Draw( mPlayer );
    Draw( mObstacle );
  }

  void ExamplePhysSim5LinCollision::Draw( const ExamplePhys5SimObj& obj )
  {
    Debug3DDrawData* drawData = mWorld->mDebug3DDrawData;
    drawData->DebugDraw3DCircle( obj.mLinPos, mCamera->mForwards, obj.mRadius, obj.mColor );

    if(spin)
        drawData->DebugDraw3DLine( obj.mLinPos,
                                   obj.mLinPos + obj.mAngRot * v3( obj.mRadius, 0, 0 ),
                                   obj.mColor );
  }


} // namespace Tac
