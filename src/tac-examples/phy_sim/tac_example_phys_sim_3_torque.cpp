#include "tac_example_phys_sim_3_torque.h" // self-inc

#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/os/tac_os.h"


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
    float s { mass / 12 };
    float ixx { s * ( b * b + c * c ) };
    float iyy { s * ( a * a + c * c ) };
    float izz { s * ( a * a + b * b ) };
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
    m3 inertiaTensor { InertiaTensorBox( mMass, v3( mWidth ) ) };
    bool inverted { inertiaTensor.Invert(&mInvMoment) };
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
    const v3 drag { -0.5f * mMass * mAngVel.Length() * mAngVel };
    return drag;
  }

  void ExamplePhysSim3Torque::Update( UpdateParams, Errors& )
  {
    ImGuiText( "WASD to apply force" );
    ImGuiCheckbox( "Draw drag force", &drawDragForce );
    ImGuiCheckbox( "Debug rot mtx", &debugPrintRotMtx );

    mForceAccumWs = {};
    mTorqueAccumWs = {};

    // Where the force is applied ( offset relative to center of mass )
    v3 wsOffset { mRot * kLocalForceOffset };

    v3 wsKeyboardForce { GetWorldspaceKeyboardDir() * 50.0f };
    v3 wsKeyboardTorque { Cross( wsOffset, wsKeyboardForce ) };
    mForceAccumWs += wsKeyboardForce;
    mTorqueAccumWs += wsKeyboardTorque;

    v3 dragForce { GetDragForce() };


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

  static String Format( float f )
  {
    String result { Tac::ToString( f ) };
    while( result.size() < 4 )
      result += " ";

    while( result.size() > 4 )
      result.pop_back();

    return result;
  }

  static void DebugPrintMtx( const m3& m )
  {
    const m3& mOrientation { m };
    v3 r0 { mOrientation.GetRow( 0 ) };
    v3 r1 { mOrientation.GetRow( 1 ) };
    v3 r2 { mOrientation.GetRow( 2 ) };

    const String s{ String()
      + Format( r0.x ) + " " + Format( r0.y ) + " " + Format( r0.z ) + "\n"
      + Format( r1.x ) + " " + Format( r1.y ) + " " + Format( r1.z ) + "\n"
      + Format( r2.x ) + " " + Format( r2.y ) + " " + Format( r2.z ) + "\n"
    };
    
    OS::OSDebugPrintLine( s );
    OS::OSDebugPrintLine( "\n" );

  }

  void ExamplePhysSim3Torque::Integrate()
  {
    const v3 a { mForceAccumWs / mMass };

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
