#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/math/tac_math.h"
#include "src/common/math/tac_matrix2.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/tac_camera.h"
#include "src/common/tac_keyboard_input.h"
#include "src/game-examples/tac_example_phys_sim_6_rotcollision.h"
#include "src/space/tac_world.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/...

static bool dorot = false;
static bool drawCapsules = false;
static bool drawCapsuleCollision = false;
static bool drawLinPos = false;
static bool drawBoundingSphere = false;
static bool drawClosestPointLineSegmentTest = false;

namespace Tac
{

  struct Sim6CollisionResult
  {
    bool mCollided = false;
    v3 mNormal; // collision normal from obj A to obj B
    v3 mPoint; // collision point
    float mDist; // penetration distance
  };

  static bool IsBroadphaseOverlapping( const ExamplePhys6SimObj& objA, const ExamplePhys6SimObj& objB )
  {
    float rSum = objA.mBoundingSphereRadius + objB.mBoundingSphereRadius;
    float rSumSq = rSum * rSum;
    float q = Quadrance( objA.mLinPos, objB.mLinPos );
    bool result = rSumSq < q;
    return result;
  }

  // optimized, no?


  // inputs:
  //   l1b - line 1 begin
  //   l1e - line 1 end
  //   l2b - line 2 begin
  //   l2e - line 2 end
  // outputs:
  //   l1c - line 1 closest point
  //   l2c - line 2 closest point
  // returns:
  //   true if an answer exists, false otherwise
  static bool ClosestPointTwoLines( const v3& l1b,
    const v3& l1e,
    const v3& l2b,
    const v3& l2e,
    v3* l1c,
    v3* l2c )
  {
    v3 l1d = l1e - l1b;
    v3 l2d = l2e - l2b;
    m2 m =
    {
      -Dot( l1d, l2d ), Dot( l2d, l2d ),
      -Dot( l1d, l1d ), Dot( l1d, l2d ),
    };

    bool exists;
    m2 minv;
    m.Invert( &exists, &minv );
    if( !exists )
      return false;

    v2 b{ Dot( l1b,l2d ) - Dot( l2d, l2b ),
          Dot( l1b,l1d ) - Dot( l2b, l1d ) };

    //v2 t = minv * b;
    auto [t1, t2] = minv * b;
    //auto [t1, t2 ] = minv * v2(Dot(l1b,l2d) - Dot(l2d, l2b)a,
    if( t1 >= 0 && t1 <= 1 &&
        t2 >= 0 && t2 <= 1 )
    {
      *l1c = l1b + t1 * l1d;
      *l2c = l2b + t2 * l2d;
      return true;
    }

    //ClosestPointLineSegment( l1b, l1e, 
    //else
    {
    }

    return true;
  }

  // should be called Sim6LineSegment?
  struct Sim6LineSegment
  {
    v3 p0;
    v3 p1;
  };

  Sim6LineSegment SimObjToLineSegment( const ExamplePhys6SimObj& obj )
  {
    v3 localUp{ 0, 1, 0 };
    v3 capsuleUp = obj.mAngRot * localUp;
    v3 capsulePoint0 = obj.mLinPos - capsuleUp * obj.mCapsuleHeight * 0.5f;
    v3 capsulePoint1 = obj.mLinPos + capsuleUp * obj.mCapsuleHeight * 0.5f;
    return { capsulePoint0,capsulePoint1 };
  }

  static Sim6CollisionResult Sim6CollideCapsuleCapsule( const ExamplePhys6SimObj& objA,
    const ExamplePhys6SimObj& objB )
  {
    Sim6LineSegment c0 = SimObjToLineSegment( objA );
    Sim6LineSegment c1 = SimObjToLineSegment( objB );


    return {};
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
  static m3 InertiaTensorCapsule( float h, float r )
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

  void ExamplePhys6SimObj::ComputeThings()
  {
    ComputeInertiaTensor();
    mBoundingSphereRadius = mCapsuleHeight * 0.5f + mCapsuleRadius;
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

  void DrawCapsule( Sim6LineSegment cap, v3 color ,
    Debug3DDrawData* drawData,
    Camera* mCamera )
  {
      drawData->DebugDraw3DCircle( cap.p0, mCamera->mForwards, 0.1f, color );
      drawData->DebugDraw3DCircle( cap.p1, mCamera->mForwards, 0.1f, color );
      drawData->DebugDraw3DLine( cap.p0, cap.p1,  color );
  }

  void DrawDashedLine(v3 p0, v3 p1, v3 color , 
    Debug3DDrawData* drawData)
  {
    v3 t = p1 - p0;
    float n = 10;
    t /= n;
    v3 p = p0;
    for( int i = 0; i < n; ++i )
    {
      v3 q = p + t;
      float f = i % 2 ? 1 : 0.1f;
      drawData->DebugDraw3DLine( p, q, color * f );
      p = q;
    }
  }

  void DrawCapsuleCapsuleCollision( Sim6LineSegment cap0,
    v3 color0,
    Sim6LineSegment cap1,
    v3 color1,
    Debug3DDrawData* drawData,
    Camera* mCamera )
  {
    v3 closest0{};
    v3 closest1{};
    bool closestExists = ClosestPointTwoLines( cap0.p0, cap0.p1, cap1.p0, cap1.p1, &closest0, &closest1 );


    v3 colors[] = { color0, color1 };
    Sim6LineSegment* caps[] = { &cap0, &cap1 };
    v3 closests[] = { closest0, closest1 };
    for( int i = 0; i < 2; ++i )
    {
      v3 color = colors[ i ];
      Sim6LineSegment* cap = caps[ i ];
      DrawCapsule(*cap, color, drawData, mCamera);
      v3 closest = closests[ i ];

      if( closestExists )
      {
        drawData->DebugDraw3DCircle( closest, mCamera->mForwards, 0.1f, color );
      }
    }

    if( closestExists )
    {
      v3 color = ( color0 + color1 ) / 2;
      DrawDashedLine(closest0, closest1, color, drawData);
      //drawData->DebugDraw3DLine( closest0, closest1, color );
    }
  }

  void ExamplePhysSim6RotCollision::RenderBoundingSpheres()
  {
    if( !drawBoundingSphere )
      return;
    Debug3DDrawData* drawData = mWorld->mDebug3DDrawData;
    bool broadphaseoverlapping = IsBroadphaseOverlapping( mPlayer, mObstacle );
    v3 color = broadphaseoverlapping ? v3( 0, 1, 0 ) : v3( 1, 0, 0 );
    for( const ExamplePhys6SimObj* obj : { &mPlayer, &mObstacle } )
      drawData->DebugDraw3DCircle( obj->mLinPos, mCamera->mForwards, obj->mBoundingSphereRadius, color );
  }

  void ExamplePhysSim6RotCollision::RenderCapsuleCollsion()
  {
    if( !drawCapsuleCollision )
      return;
    Debug3DDrawData* drawData = mWorld->mDebug3DDrawData;
    Sim6LineSegment cap0 = SimObjToLineSegment( mPlayer );
    Sim6LineSegment cap1 = SimObjToLineSegment( mObstacle );
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
    ImGuiCheckbox("Do rot", &dorot);
    ImGuiCheckbox("draw capsules", &drawCapsules);
    ImGuiCheckbox("draw capsule collision", &drawCapsuleCollision);
    ImGuiCheckbox("draw lin pos", &drawLinPos);
    ImGuiCheckbox("draw bounding sphere", &drawBoundingSphere);
    ImGuiCheckbox("test line segment", &drawClosestPointLineSegmentTest);
    if(ImGuiButton("stop moving"))
    {
      for( ExamplePhys6SimObj* obj : simobjs )
      {
        obj->mLinVel = {};
        obj->mAngVel = {};
      }
    }
  }

  void ExamplePhysSim6RotCollision::TestLineSegmentPoint()
  {
    if(!drawClosestPointLineSegmentTest)
      return;

    static v3 p0 {-.6f, -.4f, 0};
    static v3 p1 {-.2f, .8f, 0};
    static v3 p {};

    float r = 1.4f;
    float speed = 2.0f;
    p.x = ( float )Cos( speed * ShellGetElapsedSeconds() ) * r;
    p.y = ( float )Sin( speed * ShellGetElapsedSeconds() ) * r;

    p += GetWorldspaceKeyboardDir() * 0.1f;

    v3 lineColor = v3 ( 0.2f, 0.8f, 0.3f ) * 0.5f;
    v3 pointColor = v3( 0.6f, 0.3f, 0.4f );
    v3 closest = ClosestPointLineSegment( p0, p1, p );

    Debug3DDrawData* drawData = mWorld->mDebug3DDrawData;

    drawData->DebugDraw3DCircle( p0, mCamera->mForwards, 0.05f, lineColor );
    drawData->DebugDraw3DCircle( p1, mCamera->mForwards, 0.05f, lineColor );
    drawData->DebugDraw3DLine( p0, p1, lineColor );

    drawData->DebugDraw3DCircle( p, mCamera->mForwards, 0.1f, pointColor );

    drawData->DebugDraw3DCircle( closest, mCamera->mForwards, 0.03f, pointColor );
    DrawDashedLine( closest, p, pointColor, drawData);
    drawData->DebugDraw3DCircle( v3(0,0,0), mCamera->mForwards, r, pointColor * 0.2f );
    
  }

  void ExamplePhysSim6RotCollision::Update( Errors& )
  {
    m2UnitTest();

    mPlayer.BeginFrame();
    mObstacle.BeginFrame();

    const v3 keyboardForce = GetWorldspaceKeyboardDir() * 9;
    mPlayer.AddForce( keyboardForce );

    ExamplePhys6SimObj* simobjs[] = { &mPlayer, &mObstacle };
    for(ExamplePhys6SimObj* obj : simobjs )
      obj->Integrate();

    const Sim6CollisionResult collisionResult = Sim6Collide( mPlayer, mObstacle );
    Sim6ResolveCollision( collisionResult, mPlayer, mObstacle );

    Render();

    if( dorot )
      mPlayer.mAngRot = m3::RotRadZ( ( float )ShellGetElapsedSeconds() );

    TestLineSegmentPoint();
    UI();
  }

  void ExamplePhysSim6RotCollision::Draw( const ExamplePhys6SimObj& obj )
  {
    Debug3DDrawData* drawData = mWorld->mDebug3DDrawData;

    if( drawCapsules )
    {
      Sim6LineSegment cap = SimObjToLineSegment( obj );
      drawData->DebugDraw3DCapsule( cap.p0, cap.p1, obj.mCapsuleRadius, obj.mColor );
    }

    if( drawLinPos  )
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
