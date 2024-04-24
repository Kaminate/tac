#include "tac_camera.h" // self-inc

#include "tac-std-lib/math/tac_math.h"

namespace Tac
{

  void  Camera::SetForwards( v3 v )
  {
    const float q { v.Quadrance() };
    if( q < 0.01f )
      return;
    mForwards = v / Sqrt( q );
    mRight = Normalize( Cross( mForwards, v3( 0, 1, 0 ) ) );
    mUp = Cross( mRight, mForwards );
  }

  m4    Camera::View() const
  {
    return m4::View( mPos, mForwards, mRight, mUp );
  }

  //m4    Camera::Proj( float a, float b, float aspect )const
  //{
  //  return m4::ProjPerspective( a, b, mFovyrad, aspect );
  //}

  m4    Camera::ViewInv() const
  {
    return m4::ViewInv( mPos, mForwards, mRight, mUp );
  }

  //m4    Camera::ProjInv( float a, float b, float aspect )const
  //{
  //  return m4::ProjPerspectiveInv( a, b, mFovyrad, aspect );
  //}

  //Camera* debugCamera = nullptr;
}
