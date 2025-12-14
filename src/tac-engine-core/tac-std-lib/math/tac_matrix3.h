#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{
#define UseQuat() false


#if UseQuat()
  struct TmpQuat
  {
    float w, x, y, z;
    TmpQuat(const m3&);
    void Normalize();
    void Zero();

    float& operator[]( int );
    float operator[]( int ) const;
  };
#endif

  // Row-major order
  struct m3
  {
    float
      m00{ 1 }, m01{ 0 }, m02{ 0 },
      m10{ 0 }, m11{ 1 }, m12{ 0 },
      m20{ 0 }, m21{ 0 }, m22{ 1 };

#if UseQuat()
    m3(TmpQuat);
#endif

    m3() = default;
    m3( float mm00, float mm01, float mm02,
        float mm10, float mm11, float mm12,
        float mm20, float mm21, float mm22 );
    auto data() dynmc -> dynmc float*;
    auto data() const -> const float*;
    auto Determinant() const -> float;
    auto Trace() const -> float;
    auto GetColumn( int ) const -> v3;
    auto GetRow( int ) const -> v3;
    bool Invert( m3* ) const;
    auto Adjugate() const -> m3; // returns the transpose of the cofactor matrix
    auto Cofactor() const -> m3; // returns a matrix where each element is the deteriminant of...
    void Transpose();
    void OrthoNormalize();
    auto operator()( int iRow, int iCol ) dynmc -> dynmc float&;
    auto operator()( int iRow, int iCol ) const -> const float&;
    auto operator[]( int ) dynmc -> dynmc float&;
    auto operator[]( int ) const -> const float&;
    void operator *= ( float );
    void operator += ( const m3& );
    bool operator == ( const m3& ) const = default;
    bool operator != ( const m3& ) const = default;
    static auto Transpose( const m3& ) -> m3;
    static auto FromColumns( const v3&, const v3&, const v3& ) -> m3;
    static auto FromRows( const v3&, const v3&, const v3& ) -> m3;
    static auto Identity() -> m3;
    static auto Scale( float, float, float = 1) -> m3;
    static auto Scale( const v3& ) -> m3;
    static auto RotRadEuler( const v3& ) -> m3;
    static auto RotRadEulerInv( const v3& ) -> m3;
    static auto RotRadX( float ) -> m3;
    static auto RotRadY( float ) -> m3;
    static auto RotRadZ( float ) -> m3;
    static auto RotRadAngleAxis( float, const v3& ) -> m3;
    static auto Translate( float, float ) -> m3;
    static auto Translate( const v2& ) -> m3;
    static auto CrossProduct( float, float, float ) -> m3;
    static auto CrossProduct( const v3& ) -> m3;
  };

  auto operator*( float, const m3& ) -> m3;
  auto operator*( const m3&, const v3& ) -> v3;
  auto operator*( const m3&, const m3& ) -> m3;

  void m3UnitTest();

} // namespace Tac

