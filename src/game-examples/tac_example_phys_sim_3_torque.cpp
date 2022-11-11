#include "src/game-examples/tac_example_phys_sim_3_torque.h"
#include "src/space/tac_world.h"
#include "src/common/tac_camera.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/math/tac_math.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/tac_keyboard_input.h"
#include "src/common/shell/tac_shell_timer.h"

// delete me begin
#include "windows.h"
// delete me en

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/...

namespace Tac
{
  // Where the force is applied, in the local space of the object
  const v3 kLocalForceOffset{1,1,1};
  static bool drawDragForce;

  static m3 InertiaTensorBox(float mass, float a, float b, float c )
  {
    float s = mass / 12;
    float ixx = s * ( b * b + c * c );
    float iyy = s * ( a * a + c * c );
    float izz = s * ( a * a + b * b );
    return { ixx, 0, 0,
             0, iyy, 0,
             0, 0, izz };
  }
  // Assumptions:
  // - the box is of uniform density
  // - 'mass' is of the entire box
  // - 'size'.xyz is the full width/height/depth of the box size length
  static m3 InertiaTensorBox(float mass, v3 size )
  {
    return InertiaTensorBox(mass, size.x, size.y, size.z);
  }

  ExamplePhysSim3Torque::ExamplePhysSim3Torque()
  {
    m3UnitTest();
    v3UnitTest();
    m3 inertiaTensor = InertiaTensorBox( mMass, v3( mWidth ) );
    bool inverted = inertiaTensor.Invert(&mInverseMoments);
    TAC_ASSERT(inverted);
    mCamera->mPos = v3( 0, 2, -10 );
    mCamera->mPos = v3( 0, 2, 10 );
  }

  ExamplePhysSim3Torque::~ExamplePhysSim3Torque()
  {

  }

  v3 ExamplePhysSim3Torque::GetKeyboardForce()
  {
    v3 wsKeyboardForce{}; // worldspace keyboard force
    wsKeyboardForce += KeyboardIsKeyDown( Key::W ) ? mCamera->mUp : v3{};
    wsKeyboardForce += KeyboardIsKeyDown( Key::A ) ? -mCamera->mRight : v3{};
    wsKeyboardForce += KeyboardIsKeyDown( Key::S ) ? -mCamera->mUp : v3{};
    wsKeyboardForce += KeyboardIsKeyDown( Key::D ) ? mCamera->mRight : v3{};
    float q = wsKeyboardForce.Quadrance();
    if( q )
      wsKeyboardForce /= Sqrt( q );
    wsKeyboardForce *= 50.0f;

    //static v3 wsKeyForcePrev;
    //if( !q )
    //  wsKeyboardForce = wsKeyForcePrev - wsKeyForcePrev * (0.5f) * TAC_DELTA_FRAME_SECONDS;
    //wsKeyForcePrev = wsKeyboardForce;

    return wsKeyboardForce;
  }

  v3 ExamplePhysSim3Torque::GetDragForce()
  {
    return 1.5f * mMass * -mVelocity.Length() * mVelocity;
  }

  v3 ExamplePhysSim3Torque::GetDragTorque()
  {
    // fake angular friction ( should actually use moment of inertia idk )
    v3 drag = -0.5f * mMass * mAngularVelocity.Length() * mAngularVelocity;
    return drag;
  }

  void ExamplePhysSim3Torque::Update( Errors& )
  {
    ImGuiCheckbox("Draw drag force", &drawDragForce);

    mForceAccumWs = {};
    mTorqueAccumWs = {};

    // Where the force is applied ( offset relative to center of mass )
    v3 wsOffset = mOrientation * kLocalForceOffset;

    v3 wsKeyboardForce = GetKeyboardForce();
    v3 wsKeyboardTorque = Cross( wsOffset, wsKeyboardForce );
    mForceAccumWs += wsKeyboardForce;
    mTorqueAccumWs += wsKeyboardTorque;

    v3 dragForce = GetDragForce();


    mForceAccumWs += dragForce;
    mTorqueAccumWs += GetDragTorque();

    Integrate();

    mWorld->mDebug3DDrawData->DebugDraw3DCube(mPosition, mWidth,  mOrientation );
    mWorld->mDebug3DDrawData->DebugDraw3DCircle(mPosition + wsOffset, mCamera->mForwards, 0.10f );

    mWorld->mDebug3DDrawData->DebugDraw3DArrow(
      mPosition + wsOffset,
      mPosition + wsOffset + wsKeyboardForce, v3( 0, 1, 0 ) );
    if(drawDragForce)
    mWorld->mDebug3DDrawData->DebugDraw3DArrow(
      mPosition,
      mPosition + dragForce,
      v3( 1, 0, 0 ) );
  }

  // Ortho normalizes a matrix
  static void GramSchmidt( m3& m )
  {
    v3 x = m.GetColumn( 0 );
    x.Normalize();

    v3 y = m.GetColumn( 1 );
    y -= Project( x, y );
    y.Normalize();

    v3 z = m.GetColumn( 2 );
    z -= Project( x, z );
    z -= Project( y, z );
    z.Normalize();

    m = m3::FromColumns( x, y, z );
  }
  
  void ExamplePhysSim3Torque::Integrate()
  {
    v3 a = mForceAccumWs / mMass;

    mVelocity += a * TAC_DELTA_FRAME_SECONDS;
    mPosition += mVelocity * TAC_DELTA_FRAME_SECONDS;

    // !!!
    mAngularMomentum += mTorqueAccumWs * TAC_DELTA_FRAME_SECONDS;
    mAngularVelocity = mOrientation * mInverseMoments * m3::Transpose( mOrientation ) * mAngularMomentum;

    // !!!
    mOrientation += TAC_DELTA_FRAME_SECONDS * (m3::CrossProduct(mAngularMomentum) * mOrientation);

    GramSchmidt( mOrientation );

    if( mAngularVelocity.x > 0 ||
      mAngularVelocity.y > 0 ||
      mAngularVelocity.z > 0 )
    {
      v3 r0 = mOrientation.GetRow(0);
      v3 r1 = mOrientation.GetRow(1);
      v3 r2 = mOrientation.GetRow(2);
      char buf[ 1024 ];
#define FMT "%.4f "
      sprintf_s( buf,
        FMT FMT FMT "\n"
        FMT FMT FMT "\n"
        FMT FMT FMT "\n",
        r0.x, r0.y, r0.z,
        r1.x, r1.y, r1.z,
        r2.x, r2.y, r2.z );

      OutputDebugStringA( buf );
      OutputDebugStringA( "\n" );
    }

  }

} // namespace Tac
