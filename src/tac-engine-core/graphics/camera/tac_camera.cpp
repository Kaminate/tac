#include "tac_camera.h" // self-inc

#include "tac-std-lib/math/tac_math.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac
{

  static m4::ProjectionMatrixParams GetProjParams( const Camera* camera, float aspectRatio )
  {
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs{ renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera->mNearPlane },
      .mViewSpaceFar  { camera->mFarPlane },
      .mAspectRatio   { aspectRatio },
      .mFOVYRadians   { camera->mFovyrad },
    };
    return projParams;
  }

  void  Camera::SetForwards( v3 v )
  {
    const float q{ v.Quadrance() };
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

  m4    Camera::Proj( float aspectRatio ) const
  {
    const m4::ProjectionMatrixParams projParams{ GetProjParams( this, aspectRatio ) };
    return m4::ProjPerspective( projParams );
  }

  m4    Camera::ViewInv() const
  {
    return m4::ViewInv( mPos, mForwards, mRight, mUp );
  }

  m4    Camera::ProjInv( float aspectRatio ) const
  {
    const m4::ProjectionMatrixParams projParams{ GetProjParams( this, aspectRatio ) };
    return m4::ProjPerspectiveInv( projParams );
  }

}
