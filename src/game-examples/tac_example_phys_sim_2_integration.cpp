#include "src/game-examples/tac_example_phys_sim_2_integration.h"
#include "src/common/tac_camera.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/space/tac_world.h"

#include "src/common/graphics/tac_color_util.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/...

namespace Tac
{

  const char* ToString( IntegrationMode m )
  {
    switch( m )
    {
      case Euler: return "Euler";
      default: return "";
    }

  }

  ExamplePhysSim2Integration::ExamplePhysSim2Integration()
  {
    Reset();
  }

  ExamplePhysSim2Integration::~ExamplePhysSim2Integration()
  {
  }

  void ExamplePhysSim2Integration::UI()
  {
    bool shouldReset = ImGuiButton( "Reset" );
    if( ImGuiCollapsingHeader( "Change Integration Mode" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( int i = 0; i < IntegrationMode::Count; ++i )
      {
        const bool modePressed = ImGuiButton( ToString( ( IntegrationMode )i ) );
        if( modePressed )
          mIntegrationMode = ( IntegrationMode )i;
        shouldReset |= modePressed;
      }
    }
    if(shouldReset)
      Reset();
  }

  void ExamplePhysSim2Integration::Reset()
  {
    mDuration = 4.0f;

    // angular velocity is measured in radians per second.
    //   numerator:   angle rotated in radians
    //   denominator: rotation duration in seconds
    mAngularVelocity = 2.0f * 3.14f / mDuration;

    float linearVelocity =  mOrbitRadius * mAngularVelocity;

    mPosition = mCamera->mUp * mOrbitRadius;
    mVelocity = -mCamera->mRight * linearVelocity;
    timer = 0;
    mPositions.clear();
  }

  void ExamplePhysSim2Integration::TrackPositions()
  {
    timer -= TAC_DELTA_FRAME_SECONDS;
    if( timer > 0 )
      return;

    timer = 0.05f;
    if( mPositions.size() == mPositions.capacity() )
      mPositions.pop_front();
    mPositions.push_back( mPosition );
  }

  void ExamplePhysSim2Integration::DrawPositions()
  {
    if( mPositions.size() < 2 )
      return;
    v3 prev = mPositions[ 0 ];

    for( int i = 1; i < mPositions.size(); ++i )
    {
      v3 curr = mPositions[ i ];
      v4 color = v4( HSVToRGB( v3( i / (float) mPositions.size(), 1.0f, 0.5f) ), 1.0f );
      mWorld->mDebug3DDrawData->DebugDraw3DLine( prev, curr, color );
      prev = curr;
    }
  }

  v3   ExamplePhysSim2Integration::GetCentripetalAcceleration()
  {
    float radius = mPosition.Length();
    float speed = radius * mAngularVelocity;

    // centripetal acceleration
    float accelLen = speed * speed * ( 1.0f / radius );
    v3 accelDir = -Normalize( mPosition );
    v3 accel = accelDir * accelLen;
    return accel;
  }

  v3   ExamplePhysSim2Integration::GetCentripetalForce()
  {
    v3 force = mMass * GetCentripetalAcceleration();
    return force;
  }

  v3   ExamplePhysSim2Integration::GetForce()
  {
    return GetCentripetalForce();
  }

  v3   ExamplePhysSim2Integration::GetAcceleration()
  {
    return GetForce() / mMass;
  }

  void ExamplePhysSim2Integration::Update( Errors& )
  {
    UI();

    v3 accel = GetAcceleration();

    switch( mIntegrationMode)
    {
      case IntegrationMode::Euler:
      {
        mPosition += mVelocity * TAC_DELTA_FRAME_SECONDS;
        mVelocity += accel * TAC_DELTA_FRAME_SECONDS;
      } break;
    }

    TrackPositions();
    DrawPositions();
    mWorld->mDebug3DDrawData->DebugDraw3DCircle( mPosition, mCamera->mForwards, mBallRadius );
    mWorld->mDebug3DDrawData->DebugDraw3DCircle( v3{}, mCamera->mForwards, mOrbitRadius );
  }


} // namespace Tac
