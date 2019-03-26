#include "tacCamera.h"

m4 TacCamera::View()
{
  return M4View( mPos, mForwards, mRight, mUp );
}
m4 TacCamera::Proj( float a, float b, float aspect )
{
  return M4ProjPerspective( a, b, mFovyrad, aspect );
}
m4 TacCamera::ViewInv()
{
  return M4ViewInv( mPos, mForwards, mRight, mUp );
}
m4 TacCamera::ProjInv( float a, float b, float aspect )
{
  return M4ProjPerspectiveInv( a, b, mFovyrad, aspect );
}
