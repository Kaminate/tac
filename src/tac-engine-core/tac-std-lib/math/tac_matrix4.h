#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  struct v3;
  struct v4;
  struct m3;

  // Row-major order
  struct m4
  {
    float
      m00{ 1 }, m01{ 0 }, m02{ 0 }, m03{ 0 },
      m10{ 0 }, m11{ 1 }, m12{ 0 }, m13{ 0 },
      m20{ 0 }, m21{ 0 }, m22{ 1 }, m23{ 0 },
      m30{ 0 }, m31{ 0 }, m32{ 0 }, m33{ 1 };

    m4() = default;
    m4( const m3& );
    m4( float );
    m4( float, float, float, float,
        float, float, float, float,
        float, float, float, float,
        float, float, float, float );
    auto data() dynmc -> dynmc float*;
    auto data() const -> const float*;
    void Transpose();
    auto GetRow( int ) const -> v4;
    auto GetColumn( int ) const -> v4;
    void SetRow( int, const v4& );
    void SetColumn( int, const v4& );
    auto Determinant() const -> float;

    auto operator()( int iRow, int iCol ) dynmc -> dynmc float&;
    auto operator()( int iRow, int iCol ) const -> const float&;
    auto operator[]( int ) dynmc -> dynmc float&;
    auto operator[]( int ) const -> const float&;
    void operator/= ( float );
    bool operator== ( const m4& ) const;
    explicit operator m3() const;

    struct ProjectionMatrixParams
    {
      float mNDCMinZ;
      float mNDCMaxZ;
      float mViewSpaceNear;
      float mViewSpaceFar;
      float mAspectRatio;
      float mFOVYRadians;
    };

    struct OrthographicMatrixParams
    {
      float mNDCMinZ;
      float mNDCMaxZ;
      float mViewSpaceNear;
      float mViewSpaceFar;
      float mOrthoW;
      float mOrthoH;
    };

    static m4 FromRows( const v4&, const v4&, const v4&, const v4& );
    static m4 FromColumns( const v4&, const v4&, const v4&, const v4& );
    static m4 Identity();
    static m4 Inverse( const m4&, bool* resultExists );
    static m4 Scale( const v3& );
    static m4 RotRadX( float );
    static m4 RotRadY( float );
    static m4 RotRadZ( float );
    static m4 Transpose( m4 );
    static m4 Translate( const v3& translate );
    static m4 Transform( const v3& scale, const m3& rot, const v3& translate );
    static m4 Transform( const v3& scale, const v3& eulerRads, const v3& translate );
    static m4 TransformInverse( const v3& scale, const v3& eulerRads, const v3& translate );
    static m4 View( const v3& camPos, const v3& camViewDir, const v3& camR, const v3& camU );
    static m4 ViewInv( const v3& camPos, const v3& camViewDir, const v3& camR, const v3& camU );
    static m4 ProjPerspective( float A, float B, float mFieldOfViewYRad, float mAspectRatio );
    static m4 ProjPerspective( ProjectionMatrixParams );
    static m4 ProjPerspectiveInv( ProjectionMatrixParams );
    static m4 ProjPerspectiveInv( float A, float B, float mFieldOfViewYRad, float mAspectRatio );
    static m4 ProjOrthographic( OrthographicMatrixParams );
    static m4 ProjOrthographicInv( OrthographicMatrixParams );
  };

  auto operator * ( const m4&, const v4& ) -> v4;
  auto operator * ( const m4&, const m4& ) -> m4;

} // namespace Tac

