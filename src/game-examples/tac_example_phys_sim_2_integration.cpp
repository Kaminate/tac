#include "src/game-examples/tac_example_phys_sim_2_integration.h"
#include "src/common/tac_camera.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/space/tac_world.h"

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
    positions = TAC_NEW v3[ poscapacity ];

  }

  ExamplePhysSim2Integration::~ExamplePhysSim2Integration()
  {
      TAC_DELETE [] positions;
  }

  void ExamplePhysSim2Integration::UI()
  {
    for( int i = 0; i < IntegrationMode::Count; ++i )
    {
      auto mode = (IntegrationMode)i;
      if( ImGuiButton( ToString( mode ) ) )
      {
        mIntegrationMode = mode;
      }
    }
  }

  void ExamplePhysSim2Integration::Reset()
  {
    // seconds per 1 full 360 deg orbit
    float duration = 4.0f;

    // ( numerator: angle rotated in radians )
    // ( denominator: rotation duration in seconds )
    float angularVelocity = 2.0f * 3.14f / duration;

    float linearVelocity =  mOrbitRadius * angularVelocity;

    mPosition = mCamera->mUp * mOrbitRadius;
    mVelocity = -mCamera->mRight * linearVelocity;
    timer = 0;
    poscount = 0;
  }

  void ExamplePhysSim2Integration::TrackPositions()
  {

    bool addpos = false;
    timer += TAC_DELTA_FRAME_SECONDS;
    if( timer > 0.5f )
    {
      addpos = true;
      timer = 0;
    }

    if( addpos )
    {
      if( poscount == poscapacity )
      {
        iposition = ( iposition + 1 ) % poscapacity;
        poscount--;
      }
      positions[ ( iposition + poscount ) % poscapacity ] = mPosition;
      ++poscount;

    }
  }

  void ExamplePhysSim2Integration::DrawPositions()
  {
    if( poscount < 2 )
      return;
    v3 prev = positions[ iposition ];
    for( int i = 1; i < poscount; ++i )
    {
      v3 curr = positions[ ( iposition + 1 ) % poscapacity ];
      mWorld->mDebug3DDrawData->DebugDraw3DLine( prev, curr );
      prev = curr;


    }

  }

  void ExamplePhysSim2Integration::Update( Errors& )
  {
    UI();

    v3 accel{};

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

  const char* ExamplePhysSim2Integration::GetName() const { return "Phys Sim 2 Integration"; }

} // namespace Tac
