#pragma once

#include "src/game-examples/tac_examples.h"
#include "src/common/math/tac_vector3.h"

namespace Tac
{
  struct Errors;
  struct ExamplePhysSim1Force : public Example
  {
    struct Ball
    {
      float mRadius;
      float mMass;

      v3 mPos;
      v3 mVelocity;
    };

    ExamplePhysSim1Force();
    ~ExamplePhysSim1Force() override;
    void Update( Errors& ) override;
    const char* GetName() const override;

    struct Entity* mEntity = nullptr;
    struct Model* mModel = nullptr;

    Ball mBall;
    float mSpringConstant;
  };


}
