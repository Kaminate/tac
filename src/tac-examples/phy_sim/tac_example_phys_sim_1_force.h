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
      float mRadius   { .7f };
      float mMass     { 10 };
      v3    mPos      {};
      v3    mVelocity {};
    };

    void Update( Errors& ) override;

    Entity* mEntity         {};
    Model*  mModel          {};
    Ball    mBall           {};
    float   mSpringConstant { 100 };
  };
}
