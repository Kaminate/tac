#pragma once
#include "common/math/tacVector3.h"
#include "common/math/tacMatrix4.h"

struct TacCamera
{
  v3 mPos;
  v3 mForwards;
  v3 mRight;
  v3 mUp;
  float mFarPlane = 10000.0f;
  float mNearPlane = 0.1f;
  float mFovyrad = 100.0f * ( 3.14f / 180.0f );
  m4 View();
  m4 ViewInv();
  m4 Proj( float a, float b, float aspect );
  m4 ProjInv( float a, float b, float aspect );
};
