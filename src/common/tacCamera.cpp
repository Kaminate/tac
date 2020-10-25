#include "src/common/tacCamera.h"
namespace Tac
{
  m4 Camera::View()
  {
    return m4::View( mPos, mForwards, mRight, mUp );
  }

  m4 Camera::Proj( float a, float b, float aspect )
  {
    return m4::ProjPerspective( a, b, mFovyrad, aspect );
  }

  m4 Camera::ViewInv()
  {
    return m4::ViewInv( mPos, mForwards, mRight, mUp );
  }

  m4 Camera::ProjInv( float a, float b, float aspect )
  {
    return m4::ProjPerspectiveInv( a, b, mFovyrad, aspect );
  }
}
