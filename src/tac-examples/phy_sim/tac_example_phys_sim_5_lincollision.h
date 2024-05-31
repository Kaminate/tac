#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_matrix3.h"

namespace Tac
{

  struct ExamplePhys5SimObj
  {
    ExamplePhys5SimObj();

    // should be called when the radius or mass changes
    void        ComputeInertiaTensor();
    void        AddForce( v3 );
    void        BeginFrame();
    void        Integrate();
    float       Volume();
    void        Recompute();

    const char* mName                { "" };
    float       mMass                { 5 };
    float       mElasticity          { 0.5f };
    float       mRadius              { 1.0f };
    v3          mColor               {};
    v3          mLinPos              {};
    v3          mLinVel              {};
    v3          mLinForceAccum       {};
    m3          mAngRot              { m3::Identity() };
    m3          mAngInvInertiaTensor {};
    v3          mAngVel              {};
    v3          mAngMomentum         {};
    v3          mAngTorqueAccum      {};
  };


  struct ExamplePhysSim5LinCollision : public Example
  {
    ExamplePhysSim5LinCollision();
    void Update( UpdateParams, Errors& ) override;
    void DrawObject( const ExamplePhys5SimObj& );

    ExamplePhys5SimObj mPlayer;
    ExamplePhys5SimObj mObstacle;
  };
}
