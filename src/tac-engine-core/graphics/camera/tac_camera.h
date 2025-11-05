#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_matrix4.h"

namespace Tac
{
  struct Camera
  {
    void  SetForwards( v3 ); // does not need to be normalized
    m4    View() const;
    m4    ViewInv() const;
    m4    Proj( float aspectRatio ) const;
    m4    ProjInv( float aspectRatio ) const;

    v3    mPos       { 0,0,5 }; // worldspace
    v3    mForwards  { 0,0,-1}; // worldspace
    v3    mRight     { 1,0,0 }; // worldspace
    v3    mUp        { 0,1,0 }; // worldspace
    float mFarPlane  { 10000.0f };
    float mNearPlane { 0.1f };
    float mFovyrad   { 60.0f * ( 3.14f / 180.0f ) }; // Entire vertical fov (not half of it)
  };
}
