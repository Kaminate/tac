#pragma once
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacMatrix4.h"

namespace Tac
{
  struct Camera
  {
    v3    mPos = { 0,0,5 };
    v3    mForwards = { 0,-1,0 };
    v3    mRight = { 1,0,0 };
    v3    mUp = { 0,1,0 };
    float mFarPlane = 10000.0f;
    float mNearPlane = 0.1f;
    float mFovyrad = 60.0f * ( 3.14f / 180.0f );
    m4    View();
    m4    ViewInv();
    m4    Proj( float a, float b, float aspect );
    m4    ProjInv( float a, float b, float aspect );
  };
}
