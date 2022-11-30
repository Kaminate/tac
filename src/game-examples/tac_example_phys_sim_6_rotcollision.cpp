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
    return {};
    //const v3 dx = objB.mLinPos - objA.mLinPos; // vector from objA to objB
    //const float q = dx.Quadrance(); // quadrance between circles
    //const float rSum = objA.mRadius + objB.mRadius;
    //if( q > rSum * rSum )
    //  return {};
    //
    //const float d = Sqrt(q);
    //const v3 n = dx / d;
    //const float penetrationDist = rSum - d;
    //const v3 ptA = objA.mLinPos + objA.mRadius * n;
    //const v3 ptB = objB.mLinPos - objB.mRadius * n;
    //const v3 pt = ( ptA + ptB ) / 2;
    //return {
    //  .mCollided = true,
    //  .mNormal = n,
    //  .mPoint = pt,
    //  .mDist = penetrationDist
    //};
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

  // h - distance between hemisphere origins
  // r - radius of hemisphere
  // The capsule is centered in the middle, with the cylindar main axis as y-axis
  static m3 InertiaTensorCapsule(float h, float r)
  {
    // mass of the cylindar
    float mcy = h * r * r * 3.14f;

    // mass of each hemisphere
    float mhs = ( 2.0f / 3.0f ) * r * r * r * 3.14f;

    float ixx = mcy * ( ( 1 / 12.0f ) * h * h
                      + ( 1 / 4.0f ) * r * r )
              + 2 * mhs * ( ( 2 / 5.0f ) * r * r
                          + ( 1 / 2.0f ) * h * h
                          + ( 3 / 8.0f ) * h * r );
    float iyy = mcy * ( ( 1 / 2.0f ) * r * r )
              + 2 * mhs * ( ( 2 / 5.0f ) * r * r );
    float izz = ixx;
    return { ixx, 0, 0,
             0, iyy, 0,
             0, 0, izz };
  }

  void ExamplePhys6SimObj::ComputeInertiaTensor()
  {
    const m3 inertiaTensor = InertiaTensorCapsule( mCapsuleHeight, mCapsuleRadius );
    const bool inverted = inertiaTensor.Invert( &mAngInvInertiaTensor );
    TAC_ASSERT( inverted );
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
    mPlayer.mMass = 5;
    mPlayer.mLinPos = {-2, -2, 0};
    //mPlayer.mLinPos = {};
    mPlayer.mCapsuleHeight = 3.0f;
    mPlayer.mCapsuleRadius = 0.5f;
    mPlayer.mElasticity = 0.75f;
    mPlayer.mAngRot = m3::RotRadZ( 3.14f / 2.0f );
    mPlayer.mColor = v3{ 37, 150, 190 } / 255.0f;
    mPlayer.ComputeInertiaTensor();

    mObstacle.mLinPos = { 0, 3, 0 };
    //mObstacle.mLinPos = {};
    mObstacle.mCapsuleRadius = 1.0f;
    mObstacle.mCapsuleHeight = 2.0f;
    mObstacle.mMass = 10;
    mObstacle.mElasticity = 0.25f;
    mObstacle.mColor = v3{ 224, 49, 92 } / 255.0f;
    mObstacle.ComputeInertiaTensor();
  }

  ExamplePhysSim6RotCollision::~ExamplePhysSim6RotCollision()
  {

  }

  void ExamplePhysSim6RotCollision::Update( Errors& )
  {
    mPlayer.BeginFrame();
    mObstacle.BeginFrame();

    const v3 keyboardForce = GetWorldspaceKeyboardDir() * 9;

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
    drawData->DebugDraw3DCircle( obj.mLinPos, mCamera->mForwards, 0.1f * obj.mCapsuleRadius, 2.0f * obj.mColor );
    v3 wsUp = obj.mAngRot * v3{ 0,1,0 };
    v3 p0 = obj.mLinPos + wsUp * obj.mCapsuleHeight * 0.5f;
    v3 p1 = obj.mLinPos - wsUp * obj.mCapsuleHeight * 0.5f;
    drawData->DebugDraw3DCapsule( p0, p1, obj.mCapsuleRadius, obj.mColor );

#if 0
    v3 line = obj.mAngRot * v3( obj.mRadius, 0, 0 );;
    drawData->DebugDraw3DLine( obj.mLinPos, obj.mLinPos + line, obj.mColor );
#endif
  }


} // namespace Tac
