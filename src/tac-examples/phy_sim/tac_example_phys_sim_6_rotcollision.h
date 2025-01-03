#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_matrix3.h"

namespace Tac
{
  struct ExamplePhys6SimObj
  {
    ExamplePhys6SimObj();

    // should be called when the radius or mass changes
    void ComputeThings();
    void ComputeInertiaTensor();
    void AddForce(v3);
    void BeginFrame();
    void Integrate();

    float mMass                 { 5 };
    float mElasticity           { 0.5f };

    // distance between hemisphere centers
    float mCapsuleHeight        { 3 };
    float mCapsuleRadius        { 0.5f };
    float mBoundingSphereRadius {};

    v3 mColor                   {1,1,1};

    v3 mLinPos                  {};
    v3 mLinVel                  {};
    v3 mLinForceAccum           {}; // worldspace

    m3 mAngRot                  { m3::Identity() };
    m3 mAngInvInertiaTensor     {};
    v3 mAngVel                  {};
    v3 mAngMomentum             {};
    v3 mAngTorqueAccum          {}; // worldspace

  };

  struct ExamplePhysSim6RotCollision : public Example
  {
    ExamplePhysSim6RotCollision();
    ~ExamplePhysSim6RotCollision() override;

    void Update( Errors& ) override;
    void Draw( const ExamplePhys6SimObj& );
    void Render();
    void RenderBoundingSpheres();
    void RenderCapsuleCollsion();
    void UI();
    void TestLineSegmentPoint();

    ExamplePhys6SimObj mPlayer;
    ExamplePhys6SimObj mObstacle;
  };
}
