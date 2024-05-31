#pragma once

#include "tac-examples/tac_examples.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct Entity;
  struct Model;
}

namespace Tac
{
  struct ExamplePhysSim1Force : public Example
  {
    struct Ball
    {
      float mRadius   {};
      float mMass     {};
      v3    mPos      {};
      v3    mVelocity {};
    };

    ExamplePhysSim1Force();
    ~ExamplePhysSim1Force() override;
    void Update( UpdateParams, Errors& ) override;

    Entity* mEntity         {};
    Model*  mModel          {};
    Ball    mBall           {};
    float   mSpringConstant {};
  };
}
