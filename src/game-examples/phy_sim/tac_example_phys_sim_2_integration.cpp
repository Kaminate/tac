#include "src/common/containers/tac_vector.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_color_util.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/game-examples/phy_sim/tac_example_phys_sim_2_integration.h"
#include "src/space/tac_world.h"

// This example based off
// https://github.com/jvanverth/essentialmath/tree/master/src/Examples/Ch13-Simulation/...

namespace Tac
{
  float ExamplePhysSim2Integration::mBallRadius;
  float ExamplePhysSim2Integration::mOrbitRadius;
  float ExamplePhysSim2Integration::mDuration;
  float ExamplePhysSim2Integration::mMass;
  float ExamplePhysSim2Integration::mAngularVelocity;

  const char* ToString( IntegrationMode m )
  {
    switch( m )
    {
    case IntegrationMode::Euler: return "Euler";
    case IntegrationMode::SemiImplicitEuler: return "Semi-implicit Euler";
    case IntegrationMode::RK4: return "Runge-Kutta 4";
      default: TAC_CRITICAL_ERROR_INVALID_CASE( m ); return "";
    }
  }

  struct Rk4StateDerivative
  {
    v3 mVelocity;
    v3 mAcceleration;
  };

  Rk4StateDerivative operator * ( float f, const Rk4StateDerivative& k )
  {
    return Rk4StateDerivative
    {
      .mVelocity = f * k.mVelocity,
      .mAcceleration = f * k.mAcceleration,
    };
  }

  Rk4StateDerivative operator + ( const Rk4StateDerivative& lhs, const Rk4StateDerivative& rhs )
  {
    return Rk4StateDerivative
    {
      .mVelocity = lhs.mVelocity + rhs.mVelocity,
      .mAcceleration = lhs.mAcceleration + rhs.mAcceleration,
    };
  }

  struct RK4State
  {
    v3 mPosition;
    v3 mVelocity;

    Rk4StateDerivative Derive() const
    {
      return
      {
        .mVelocity = mVelocity,
        .mAcceleration = ExamplePhysSim2Integration::GetAcceleration( mPosition )
      };
    }

    RK4State operator + ( const Rk4StateDerivative& k ) const
    {
      return
      {
        .mPosition = mPosition + k.mVelocity,
        .mVelocity = mVelocity + k.mAcceleration,
      };
    }

  };

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
    ImGuiTextf( "Current Mode: %s", ToString( mIntegrationMode ) );

    ImGuiText( "Change Mode:" );
    for( int i = 0; i < (int)IntegrationMode::Count; ++i )
    {
      ImGuiSameLine();
      const bool modePressed = ImGuiButton( ToString( ( IntegrationMode )i ) );
      if( modePressed )
        mIntegrationMode = ( IntegrationMode )i;
      shouldReset |= modePressed;
    }

    if(shouldReset)
      Reset();
  }

  void ExamplePhysSim2Integration::Reset()
  {
    mBallRadius = 0.2f;
    mOrbitRadius = 1;
    mMass = 10;
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

  v3   ExamplePhysSim2Integration::GetCentripetalAcceleration(v3 pos)
  {
    float radius = pos.Length();
    float speed = radius * mAngularVelocity;

    // centripetal acceleration
    float accelLen = speed * speed * ( 1.0f / radius );
    v3 accelDir = -Normalize( pos );
    v3 accel = accelDir * accelLen;
    return accel;
  }

  v3   ExamplePhysSim2Integration::GetCentripetalForce(v3 pos)
  {
    v3 force = mMass * GetCentripetalAcceleration(pos);
    return force;
  }

  v3   ExamplePhysSim2Integration::GetForce(v3 pos)
  {
    return GetCentripetalForce(pos);
  }


  v3   ExamplePhysSim2Integration::GetAcceleration(v3 pos)
  {
    return GetForce(pos) / mMass;
  }


  void ExamplePhysSim2Integration::Update( Errors& )
  {
    UI();

    switch( mIntegrationMode )
    {
      case IntegrationMode::Euler:
      {
        v3 accel = GetAcceleration(mPosition);
        mPosition += mVelocity * TAC_DELTA_FRAME_SECONDS;
        mVelocity += accel * TAC_DELTA_FRAME_SECONDS;
      } break;
      case IntegrationMode::SemiImplicitEuler:
      {
        v3 accel = GetAcceleration(mPosition);

        // Explicit velocity update step
        mVelocity += accel * TAC_DELTA_FRAME_SECONDS;

        // Implicit position update step
        mPosition += mVelocity * TAC_DELTA_FRAME_SECONDS;
      } break;
      case IntegrationMode::RK4:
      {
        const RK4State s1
        {
          .mPosition = mPosition,
          .mVelocity = mVelocity
        };
        const Rk4StateDerivative k1 = s1.Derive();

        const RK4State s2 = s1 + 0.5f * TAC_DELTA_FRAME_SECONDS * k1;
        const Rk4StateDerivative k2 = s2.Derive();

        const RK4State s3 = s1 + 0.5f * TAC_DELTA_FRAME_SECONDS * k2;
        const Rk4StateDerivative k3 = s3.Derive();

        const RK4State s4 = s1 + TAC_DELTA_FRAME_SECONDS * k3;
        const Rk4StateDerivative k4 = s4.Derive();

        const Rk4StateDerivative k =
          ( 1 / 6.0f ) * k1 +
          ( 2 / 6.0f ) * k2 +
          ( 2 / 6.0f ) * k3 +
          ( 1 / 6.0f ) * k4;
        const RK4State s = s1 + TAC_DELTA_FRAME_SECONDS * k;
        mPosition = s.mPosition;
        mVelocity = s.mVelocity;
      } break;
      default: TAC_CRITICAL_ERROR_INVALID_CASE( mIntegrationMode );
    }

    mWorld->mDebug3DDrawData->DebugDraw3DCircle( mPosition, mCamera->mForwards, mBallRadius );
    mWorld->mDebug3DDrawData->DebugDraw3DCircle( v3{}, mCamera->mForwards, mOrbitRadius );
    TrackPositions();
    DrawPositions();
  }


} // namespace Tac
