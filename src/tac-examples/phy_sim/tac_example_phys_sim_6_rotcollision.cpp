#include "tac_example_phys_sim_6_rotcollision.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix2.h"
#include "tac-engine-core/shell/tac_shell_game_timer.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

#include "tac-ecs/world/tac_world.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/...

static bool dorot                           {};
static bool drawCapsules                    { true };
static bool drawCapsuleCollision            {};
static bool drawLinPos                      {};
static bool drawBoundingSphere              {};
static bool drawClosestPointLineSegmentTest {};

namespace Tac
{

  struct Sim6CollisionResult
  {
    bool  mCollided {};
    v3    mNormal   {}; // collision normal from obj A to obj B
    v3    mPoint    {}; // collision point
    float mDist     {}; // penetration distance
  };


  struct Sim6LineSegment
  {
    v3 p0;
    v3 p1;
  };

  struct Sim6Capsule
  {
    Sim6LineSegment mLineSegment {};
    float           mRadius      {};
  };

  Sim6LineSegment SimObjToLineSegment( const ExamplePhys6SimObj& obj )
  {
    v3 localUp{ 0, 1, 0 };
    v3 capsuleUp { obj.mAngRot * localUp };
    v3 capsulePoint0 { obj.mLinPos - capsuleUp * obj.mCapsuleHeight * 0.5f };
    v3 capsulePoint1 { obj.mLinPos + capsuleUp * obj.mCapsuleHeight * 0.5f };
    return { capsulePoint0,capsulePoint1 };
  }

  static bool IsBroadphaseOverlapping( const ExamplePhys6SimObj& objA,
                                       const ExamplePhys6SimObj& objB )
  {
    const float rSum { objA.mBoundingSphereRadius + objB.mBoundingSphereRadius };
    const float rSumSq { rSum * rSum };
    const float q { Quadrance( objA.mLinPos, objB.mLinPos ) };
    const bool result { q < rSumSq };
    return result;
  }


  static Sim6CollisionResult Sim6CollideCapsuleCapsule( const ExamplePhys6SimObj& objA,
                                                        const ExamplePhys6SimObj& objB )
  {
    const Sim6LineSegment lineSegmentA { SimObjToLineSegment( objA ) };
    const Sim6LineSegment lineSegmentB { SimObjToLineSegment( objB ) };
    const ClosestPointLineSegments output( {
      .mLine1{ lineSegmentA.p0, lineSegmentA.p1 },
      .mLine2{ lineSegmentB.p0, lineSegmentB.p1 } } );
    const v3 closestA{ output.mClosestPointOnLine1 };
    const v3 closestB{ output.mClosestPointOnLine2 };
    const float radiusSum { objA.mCapsuleRadius + objB.mCapsuleRadius };
    const v3 v { closestB - closestA };
    const float q { v.Quadrance() };
    if( Square( radiusSum ) < q )
      return {};
    const float d { Sqrt( q ) };
    const v3 n { v / d };
    const v3 intersectionPoint{ ( ( closestA + n * objA.mCapsuleRadius ) +
                                  ( closestB - n * objB.mCapsuleRadius ) ) / 2 };
    const float penetrationDistance { radiusSum - d };
    return Sim6CollisionResult
    {
      .mCollided { true },
      .mNormal   { n },
      .mPoint    { intersectionPoint },
      .mDist     { penetrationDistance },
    };
  }

  static Sim6CollisionResult Sim6Collide( const ExamplePhys6SimObj& objA, const ExamplePhys6SimObj& objB )
  {
    if( !IsBroadphaseOverlapping( objA, objB ) )
      return {};
    return Sim6CollideCapsuleCapsule( objA, objB );
  }

  static void Sim6ResolveCollision( const Sim6CollisionResult& collisionResult,
                                    ExamplePhys6SimObj& objA,
                                    ExamplePhys6SimObj& objB )
  {
    if( !collisionResult.mCollided )
      return;

    const v3& n { collisionResult.mNormal };

    // Push out, scaling penetration distance by relative mass
    const float tA { objA.mMass / ( objA.mMass + objB.mMass ) };
    const float tB { objB.mMass / ( objA.mMass + objB.mMass ) };

    // compute relative velocity
    const v3 rA { collisionResult.mPoint - objA.mLinPos };
    const v3 rB { collisionResult.mPoint - objB.mLinPos };
    const v3 velA { objA.mLinVel + Cross( objA.mAngVel, rA ) };
    const v3 velB { objB.mLinVel + Cross( objB.mAngVel, rB ) };
    const v3 relVel { velA - velB };

    // objects are already going away
    const float vDotN { Dot( relVel, n ) };
    if( vDotN < 0 )
      return;

    const float restitution { objA.mElasticity * objB.mElasticity };

    const float j = [&](){
      const float numerator { -( 1 + restitution ) * vDotN };
      const float denomPartMass { 1.0f / objA.mMass + 1.0f / objB.mMass };
      const v3 denomPartA { Cross( objA.mAngInvInertiaTensor * Cross( rA, n ), rA ) };
      const v3 denomPartB { Cross( objB.mAngInvInertiaTensor * Cross( rB, n ), rB ) };
      const float denominator { denomPartMass + Dot( denomPartA + denomPartB, n ) };
      return numerator / denominator;
    }( );

    objA.mLinPos -= tA * collisionResult.mDist * n;
    objB.mLinPos += tB * collisionResult.mDist * n;

    objA.mLinVel += ( j / objA.mMass ) * n;
    objB.mLinVel -= ( j / objB.mMass ) * n;

    objA.mAngMomentum += Cross( rA, j * n );
    objB.mAngMomentum -= Cross( rB, j * n );

    // don't necessarily have to set it here since it will be recomputed during Integrate() but why not
    objA.mAngVel = objA.mAngInvInertiaTensor * objA.mAngMomentum;
    objB.mAngVel = objB.mAngInvInertiaTensor * objB.mAngMomentum;
  }

  ExamplePhys6SimObj::ExamplePhys6SimObj()
  {
    ComputeInertiaTensor();
  }

  // h - distance between hemisphere origins
  // r - radius of hemisphere
  // The capsule is centered in the middle, with the cylindar main axis as y-axis
  static m3 InertiaTensorCapsule( float h, float r )
  {
    // mass of the cylindar
    float mcy { h * r * r * 3.14f };

    // mass of each hemisphere
    float mhs { ( 2.0f / 3.0f ) * r * r * r * 3.14f };

    float ixx { mcy * ( ( 1 / 12.0f ) * h * h + ( 1 / 4.0f ) * r * r ) + 2 * mhs * ( ( 2 / 5.0f ) * r * r + ( 1 / 2.0f ) * h * h + ( 3 / 8.0f ) * h * r ) };
    float iyy { mcy * ( ( 1 / 2.0f ) * r * r ) + 2 * mhs * ( ( 2 / 5.0f ) * r * r ) };
    float izz { ixx };
    return { ixx, 0, 0,
             0, iyy, 0,
             0, 0, izz };
  }

  void ExamplePhys6SimObj::ComputeThings()
  {
    ComputeInertiaTensor();
    mBoundingSphereRadius = mCapsuleHeight * 0.5f + mCapsuleRadius;
  }

  void ExamplePhys6SimObj::ComputeInertiaTensor()
  {
    const m3 inertiaTensor { InertiaTensorCapsule( mCapsuleHeight, mCapsuleRadius ) };
    const bool inverted { inertiaTensor.Invert( &mAngInvInertiaTensor ) };
    TAC_ASSERT( inverted );
  }

  void ExamplePhys6SimObj::BeginFrame()
  {
    mLinForceAccum = {};
    mAngTorqueAccum = {};
  }

  void ExamplePhys6SimObj::Integrate()
  {
    const float dt { TAC_DT };
    const v3 a { mLinForceAccum / mMass };

    mLinVel += a * dt;
    mLinPos += mLinVel * dt;

    mAngMomentum += mAngTorqueAccum * TAC_DT;
    mAngVel = mAngRot * mAngInvInertiaTensor * m3::Transpose( mAngRot ) * mAngMomentum;

    mAngRot += TAC_DT * ( m3::CrossProduct( mAngVel ) * mAngRot );

    mAngRot.OrthoNormalize();
  }

  void ExamplePhys6SimObj::AddForce( v3 force )
  {
    mLinForceAccum += force;
  }

  ExamplePhysSim6RotCollision::ExamplePhysSim6RotCollision()
  {
    mCamera->mPos = { 0, 0, 8 };
    //mCamera = TAC_NEW Camera{ .mPos = { 0, 0, 5 },
    //                          .mForwards = { 0, 0, -1 },
    //                          .mRight = { 1, 0, 0 },
    //                          .mUp = { 0, 1, 0 } };

    mPlayer.mMass = 5;
    mPlayer.mLinPos = { -2, -2, 0 };
    mPlayer.mCapsuleHeight = 3.0f;
    mPlayer.mCapsuleRadius = 0.5f;
    mPlayer.mElasticity = 0.75f;
    mPlayer.mAngRot = m3::RotRadZ( 3.14f / 2.0f );
    mPlayer.mColor = v3{ 37, 150, 190 } / 255.0f;
    mPlayer.ComputeThings();

    mObstacle.mLinPos = { 0, 3, 0 };
    mObstacle.mCapsuleRadius = 1.0f;
    mObstacle.mCapsuleHeight = 2.0f;
    mObstacle.mMass = 10;
    mObstacle.mElasticity = 0.25f;
    mObstacle.mColor = v3{ 224, 49, 92 } / 255.0f;
    mObstacle.ComputeThings();
  }

  ExamplePhysSim6RotCollision::~ExamplePhysSim6RotCollision()
  {

  }

  static void DrawLineSegment( Sim6LineSegment lineSegment,
                               v3 color,
                               Debug3DDrawData* drawData,
                               Camera* mCamera )
  {
    drawData->DebugDraw3DCircle( lineSegment.p0, mCamera->mForwards, 0.1f, color );
    drawData->DebugDraw3DCircle( lineSegment.p1, mCamera->mForwards, 0.1f, color );
    drawData->DebugDraw3DLine( lineSegment.p0, lineSegment.p1, color );
  }

  static void DrawDashedLine( v3 p0,
                              v3 p1,
                              v3 color,
                              Debug3DDrawData* drawData )
  {
    v3 t { p1 - p0 };
    float n { 10 };
    t /= n;
    v3 p { p0 };
    for( int i {}; i < n; ++i )
    {
      const v3 q { p + t };
      const float f { i % 2 ? 1 : 0.1f };
      drawData->DebugDraw3DLine( p, q, color * f );
      p = q;
    }
  }

  void DrawCapsuleCapsuleCollision( Sim6Capsule capsule0,
                                    v3 color0,
                                    Sim6Capsule capsule1,
                                    v3 color1,
                                    Debug3DDrawData* drawData,
                                    Camera* mCamera )
  {
    const ClosestPointLineSegments output( {
      .mLine1 { capsule0.mLineSegment.p0, capsule0.mLineSegment.p1 },
      .mLine2 { capsule1.mLineSegment.p0, capsule1.mLineSegment.p1 } } );
    const v3 closest0{ output.mClosestPointOnLine1 };
    const v3 closest1{ output.mClosestPointOnLine2 };
    const bool intersecting { Quadrance( closest0, closest1 )
      < Square( capsule0.mRadius + capsule1.mRadius ) };
    const v3 colors[]          { color0, color1 };
    const Sim6Capsule* caps[]  { &capsule0, &capsule1 };
    const v3 closests[]        { closest0, closest1 };
    for( int i {}; i < 2; ++i )
    {
      const v3 color { colors[ i ] };
      const Sim6Capsule* cap { caps[ i ] };
      const v3 closest { closests[ i ] };
      DrawLineSegment( cap->mLineSegment, color, drawData, mCamera );
      const float rad{ 0.1f + 0.05f * ( i + 1 ) };
      drawData->DebugDraw3DCircle( closest, mCamera->mForwards, rad, color * 2.0f );
    }

    const v3 avgcolor { ( color0 + color1 ) / 2 };
    DrawDashedLine( closest0, closest1, avgcolor, drawData );
    if( intersecting )
    {
      v3 n { closest1 - closest0 };
      const float d { n.Length() };
      if( d > 0 )
        n /= d;

      const v3 collisionPt0 { closest0 + n * capsule0.mRadius };
      const v3 collisionPt1 { closest1 - n * capsule1.mRadius };
      const v3 collisionPt { ( collisionPt0 + collisionPt1 ) / 2 };
      const float penetration { d - capsule0.mRadius - capsule1.mRadius };
      drawData->DebugDraw3DCircle( collisionPt, mCamera->mForwards, penetration / 2, avgcolor );
    }
  }

  void ExamplePhysSim6RotCollision::RenderBoundingSpheres()
  {
    if( !drawBoundingSphere )
      return;
    Debug3DDrawData* drawData { mWorld->mDebug3DDrawData };
    bool broadphaseoverlapping { IsBroadphaseOverlapping( mPlayer, mObstacle ) };
    v3 color { broadphaseoverlapping ? v3( 0, 1, 0 ) : v3( 1, 0, 0 ) };
    for( const ExamplePhys6SimObj* obj : { &mPlayer, &mObstacle } )
      drawData->DebugDraw3DCircle( obj->mLinPos, mCamera->mForwards, obj->mBoundingSphereRadius, color );
  }


  Sim6Capsule SimObjToCapsule( const ExamplePhys6SimObj& obj )
  {
    return Sim6Capsule
    {
      .mLineSegment { SimObjToLineSegment( obj ) },
      .mRadius      { obj.mCapsuleRadius },
    };
  }

  void ExamplePhysSim6RotCollision::RenderCapsuleCollsion()
  {
    if( !drawCapsuleCollision )
      return;
    Debug3DDrawData* drawData { mWorld->mDebug3DDrawData };
    Sim6Capsule cap0 { SimObjToCapsule( mPlayer ) };
    Sim6Capsule cap1 { SimObjToCapsule( mObstacle ) };
    DrawCapsuleCapsuleCollision( cap0, mPlayer.mColor, cap1, mObstacle.mColor, drawData, mCamera );
  }
  void ExamplePhysSim6RotCollision::Render()
  {
    RenderBoundingSpheres();
    RenderCapsuleCollsion();
    Draw( mPlayer );
    Draw( mObstacle );
  }

  void ExamplePhysSim6RotCollision::UI()
  {
    ExamplePhys6SimObj* simobjs[] = { &mPlayer, &mObstacle };
    ImGuiCheckbox( "Do rot", &dorot );
    ImGuiCheckbox( "draw capsules", &drawCapsules );
    ImGuiCheckbox( "draw capsule collision", &drawCapsuleCollision );
    ImGuiCheckbox( "draw lin pos", &drawLinPos );
    ImGuiCheckbox( "draw bounding sphere", &drawBoundingSphere );
    ImGuiCheckbox( "test line segment", &drawClosestPointLineSegmentTest );
    if( ImGuiButton( "stop moving" ) )
    {
      for( ExamplePhys6SimObj* obj : simobjs )
      {
        obj->mLinVel = {};
        obj->mAngVel = {};
      }
    }
  }

  // visual unit test for ClosestPointLineSegment() function
  void ExamplePhysSim6RotCollision::TestLineSegmentPoint()
  {
    if( !drawClosestPointLineSegmentTest )
      return;

    static v3 p0{ -.6f, -.4f, 0 };
    static v3 p1{ -.2f, .8f, 0 };
    static v3 p{};

    const float r { 1.4f };
    const float speed { 2.0f };
    p.x = ( float )Cos( speed * GameTimer::GetElapsedTime() ) * r;
    p.y = ( float )Sin( speed * GameTimer::GetElapsedTime() ) * r;
    p += GetWorldspaceKeyboardDir() * 0.1f;
    const v3 lineColor { v3( 0.2f, 0.8f, 0.3f ) * 0.5f };
    const v3 pointColor { v3( 0.6f, 0.3f, 0.4f ) };
    const v3 closest{ ClosestPointLineSegment( LineSegment{ p0, p1 }, p ) };
    Debug3DDrawData* drawData { mWorld->mDebug3DDrawData };
    drawData->DebugDraw3DCircle( p0, mCamera->mForwards, 0.05f, lineColor );
    drawData->DebugDraw3DCircle( p1, mCamera->mForwards, 0.05f, lineColor );
    drawData->DebugDraw3DLine( p0, p1, lineColor );
    drawData->DebugDraw3DCircle( p, mCamera->mForwards, 0.1f, pointColor );
    drawData->DebugDraw3DCircle( closest, mCamera->mForwards, 0.03f, pointColor );
    DrawDashedLine( closest, p, pointColor, drawData );
    drawData->DebugDraw3DCircle( v3( 0, 0, 0 ), mCamera->mForwards, r, pointColor * 0.2f );
  }

  void ExamplePhysSim6RotCollision::Update( Errors& )
  {
    m2UnitTest();
    mPlayer.BeginFrame();
    mObstacle.BeginFrame();
    ImGuiText( "Controls: WASD" );
    const v3 keyboardForce { GetWorldspaceKeyboardDir() * 9 };
    mPlayer.AddForce( keyboardForce );
    ExamplePhys6SimObj* simobjs[]  { &mPlayer, &mObstacle };
    for( ExamplePhys6SimObj* obj : simobjs )
      obj->Integrate();

    const Sim6CollisionResult collisionResult { Sim6Collide( mPlayer, mObstacle ) };
    Sim6ResolveCollision( collisionResult, mPlayer, mObstacle );
    Render();
    if( dorot )
      mPlayer.mAngRot = m3::RotRadZ( ( float )GameTimer::GetElapsedTime() );

    TestLineSegmentPoint();
    UI();
  }

  void ExamplePhysSim6RotCollision::Draw( const ExamplePhys6SimObj& obj )
  {
    Debug3DDrawData* drawData { mWorld->mDebug3DDrawData };
    if( drawCapsules )
    {
      Sim6LineSegment cap { SimObjToLineSegment( obj ) };
      drawData->DebugDraw3DCapsule( cap.p0, cap.p1, obj.mCapsuleRadius, obj.mColor );
    }

    if( drawLinPos )
    {
      drawData->DebugDraw3DCircle( obj.mLinPos, mCamera->mForwards, 0.1f * obj.mCapsuleRadius, 2.0f * obj.mColor );
    }

    if( dorot )
    {
      //v3 line = obj.mAngRot * v3( obj.mRadius, 0, 0 );
      //drawData->DebugDraw3DLine( obj.mLinPos, obj.mLinPos + line, obj.mColor );
    }
  }


} // namespace Tac
