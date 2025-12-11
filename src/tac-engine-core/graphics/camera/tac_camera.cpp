#include "tac_camera.h" // self-inc

#include "tac-std-lib/math/tac_math.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac
{

  static auto GetProjParams( const Camera* camera, float aspectRatio ) -> m4::ProjectionMatrixParams
  {
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs{ renderDevice->GetInfo().mNDCAttribs };
    return
      m4::ProjectionMatrixParams
      {
        .mNDCMinZ       { ndcAttribs.mMinZ },
        .mNDCMaxZ       { ndcAttribs.mMaxZ },
        .mViewSpaceNear { camera->mNearPlane },
        .mViewSpaceFar  { camera->mFarPlane },
        .mAspectRatio   { aspectRatio },
        .mFOVYRadians   { camera->mFovyrad },
      };
  }

  static auto GetOrthoParams( const Camera* camera, float aspectRatio ) -> m4::OrthographicMatrixParams
  {
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs{ renderDevice->GetInfo().mNDCAttribs };
    return
      m4::OrthographicMatrixParams
      {
        .mNDCMinZ       { ndcAttribs.mMinZ },
        .mNDCMaxZ       { ndcAttribs.mMaxZ },
        .mViewSpaceNear { camera->mNearPlane },
        .mViewSpaceFar  { camera->mFarPlane },
        .mOrthoW        { camera->mOrthoHeight * aspectRatio },
        .mOrthoH        { camera->mOrthoHeight },
      };

  }

  auto Camera::TypeToString( Type type ) -> const char*
  {
    switch( type )
    {
      case kPerspective: return "Perspective";
      case kOrthographic: return "Orthographic";
      default: TAC_ASSERT_INVALID_CASE( type ); return "???";
    }
  }

  void Camera::SetForwards( v3 v )
  {
    const float q{ v.Quadrance() };
    if( q < 0.01f )
      return;

    mForwards = v / Sqrt( q );
    mRight = Normalize( Cross( mForwards, v3( 0, 1, 0 ) ) );
    mUp = Cross( mRight, mForwards );
  }

  auto Camera::View() const -> m4 { return m4::View( mPos, mForwards, mRight, mUp ); }

  auto Camera::Proj( float aspectRatio ) const -> m4
  {
    switch( mType )
    {
      case kPerspective:
        return m4::ProjPerspective( GetProjParams( this, aspectRatio ) );
      case kOrthographic:
        return m4::ProjOrthographic( GetOrthoParams( this, aspectRatio ) );
      default:
        TAC_ASSERT_INVALID_CASE( mType );
        return {};
    }
  }

  auto Camera::ViewInv() const -> m4 { return m4::ViewInv( mPos, mForwards, mRight, mUp ); }

  auto Camera::ProjInv( float aspectRatio ) const -> m4
  {
    switch( mType )
    {
      case kPerspective:
        return m4::ProjPerspectiveInv( GetProjParams( this, aspectRatio )  );
      case kOrthographic:
        return m4::ProjOrthographicInv( GetOrthoParams( this, aspectRatio )  );
      default:
        TAC_ASSERT_INVALID_CASE( mType );
        return {};
    }
  }

}
