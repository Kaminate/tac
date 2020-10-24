#include "src/common/tacCamera.h"
namespace Tac
{

  m4 Camera::View()
  {
    return M4View( mPos, mForwards, mRight, mUp );
  }
  m4 Camera::Proj( float a, float b, float aspect )
  {
    return M4ProjPerspective( a, b, mFovyrad, aspect );
  }
  m4 Camera::ViewInv()
  {
    return M4ViewInv( mPos, mForwards, mRight, mUp );
  }
  m4 Camera::ProjInv( float a, float b, float aspect )
  {
    return M4ProjPerspectiveInv( a, b, mFovyrad, aspect );
  }
}
