#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/tac_core.h"

namespace Tac
{
  struct ExamplePhysSim1Force : public Example
  {
    struct Ball
    {
      float mRadius = 0;
      float mMass = 0;

      v3 mPos;
      v3 mVelocity;
    };

    ExamplePhysSim1Force();
    ~ExamplePhysSim1Force() override;
    void Update( Errors& ) override;

    struct Entity* mEntity = nullptr;
    struct Model* mModel = nullptr;

    Ball mBall;
    float mSpringConstant;
  };


}
