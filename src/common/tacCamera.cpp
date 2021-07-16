#include "src/common/tacCamera.h"
namespace Tac
{
  m4 Camera::View() const
  {
    return m4::View( mPos, mForwards, mRight, mUp );
  }

  m4 Camera::Proj( float a, float b, float aspect )const
  {
    return m4::ProjPerspective( a, b, mFovyrad, aspect );
  }

  m4 Camera::ViewInv() const
  {
    return m4::ViewInv( mPos, mForwards, mRight, mUp );
  }

  m4 Camera::ProjInv( float a, float b, float aspect )const
  {
    return m4::ProjPerspectiveInv( a, b, mFovyrad, aspect );
  }

  Camera* debugCamera = nullptr;
}
