#include "src/game-examples/phy_sim/tac_example_phys_sim_3_torque.h"
#include "src/space/tac_world.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/math/tac_math.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/system/tac_os.h"

#define FMT "% .4f "

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/...

namespace Tac
{
  // Where the force is applied, in the local space of the object
  const v3 kLocalForceOffset{1,1,1};
  static bool drawDragForce;
  static bool debugPrintRotMtx;

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
    bool inverted = inertiaTensor.Invert(&mInvMoment);
    TAC_ASSERT(inverted);
    mCamera->mPos = v3( 0, 2, -10 );
    mCamera->mPos = v3( 0, 2, 10 );
  }

  v3 ExamplePhysSim3Torque::GetDragForce()
  {
    return 1.5f * mMass * -mLinVel.Length() * mLinVel;
  }

  v3 ExamplePhysSim3Torque::GetDragTorque()
  {
    // fake angular friction ( should actually use moment of inertia idk )
    v3 drag = -0.5f * mMass * mAngVel.Length() * mAngVel;
    return drag;
  }

  void ExamplePhysSim3Torque::Update( Errors& )
  {
    ImGuiCheckbox("Draw drag force", &drawDragForce);

    ImGuiCheckbox( "Debug rot mtx", &debugPrintRotMtx );

    mForceAccumWs = {};
    mTorqueAccumWs = {};

    // Where the force is applied ( offset relative to center of mass )
    v3 wsOffset = mRot * kLocalForceOffset;

    v3 wsKeyboardForce = GetWorldspaceKeyboardDir() * 50.0f;
    v3 wsKeyboardTorque = Cross( wsOffset, wsKeyboardForce );
    mForceAccumWs += wsKeyboardForce;
    mTorqueAccumWs += wsKeyboardTorque;

    v3 dragForce = GetDragForce();


    mForceAccumWs += dragForce;
    mTorqueAccumWs += GetDragTorque();

    Integrate();

    mWorld->mDebug3DDrawData->DebugDraw3DCube(mPos, mWidth,  mRot );
    mWorld->mDebug3DDrawData->DebugDraw3DCircle(mPos + wsOffset, mCamera->mForwards, 0.10f );

    mWorld->mDebug3DDrawData->DebugDraw3DArrow(
      mPos + wsOffset,
      mPos + wsOffset + wsKeyboardForce, v3( 0, 1, 0 ) );
    if(drawDragForce)
    mWorld->mDebug3DDrawData->DebugDraw3DArrow(
      mPos,
      mPos + dragForce,
      v3( 1, 0, 0 ) );
  }


  static void DebugPrintMtx( const m3& m )
  {
    const m3& mOrientation = m;
    v3 r0 = mOrientation.GetRow( 0 );
    v3 r1 = mOrientation.GetRow( 1 );
    v3 r2 = mOrientation.GetRow( 2 );
    const char* buf = va( 
      FMT FMT FMT "\n"
      FMT FMT FMT "\n"
      FMT FMT FMT "\n",
      r0.x, r0.y, r0.z,
      r1.x, r1.y, r1.z,
      r2.x, r2.y, r2.z );

    
    OS::OSDebugPrintLine( buf );
    OS::OSDebugPrintLine( "\n" );

    ImGuiTextf( 
      FMT FMT FMT "\n"
      FMT FMT FMT "\n"
      FMT FMT FMT "\n",
      r0.x, r0.y, r0.z,
      r1.x, r1.y, r1.z,
      r2.x, r2.y, r2.z );
  }

  void ExamplePhysSim3Torque::Integrate()
  {
    const v3 a = mForceAccumWs / mMass;

    mLinVel += a * TAC_DELTA_FRAME_SECONDS;
    mPos += mLinVel * TAC_DELTA_FRAME_SECONDS;

    mAngMomentum += mTorqueAccumWs * TAC_DELTA_FRAME_SECONDS;
    mAngVel = mRot * mInvMoment * m3::Transpose( mRot ) * mAngMomentum;

    mRot += TAC_DELTA_FRAME_SECONDS * ( m3::CrossProduct( mAngVel ) * mRot );

    mRot.OrthoNormalize();

    if( debugPrintRotMtx && ( mAngVel.x > 0 || mAngVel.y > 0 || mAngVel.z > 0 ) )
    {
      DebugPrintMtx( mRot );
    }

  }

} // namespace Tac
