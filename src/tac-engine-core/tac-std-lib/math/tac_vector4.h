#pragma once
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector2.h"

namespace Tac
{
  struct v4
  {
    float x {};
    float y {};
    float z {};
    float w {};
    v4() = default;
    v4( float );
    v4( float, float, float, float );
    v4( const v3& xyz, float w );
    v4( const v2& xy, float z, float w );
    auto begin() -> float*;
    auto end() -> float*;
    auto data() -> float*;
    auto data() const -> const float*;
    auto xy() -> v2&;
    auto xy() const -> const v2&;
    auto xyz() -> v3&;
    auto xyz() const -> const v3&;
    void Normalize();
    auto Length() const -> float;
    auto Quadrance() const -> float;
    auto operator[]( int ) -> float&;
    auto operator[]( int ) const -> float;
    void operator /= ( float );
    void operator *= ( float );
    void operator -= ( const v4& );
    void operator += ( const v4& );
    bool operator == ( const v4& )const;
    bool operator != ( const v4& )const;
    auto operator - () const -> v4;
    auto operator * ( float ) const -> v4;
    auto operator / ( float ) const -> v4;
    auto operator + ( const v4& ) const -> v4;
    auto operator - ( const v4& ) const -> v4;
  };

  auto operator*( float, const v4& ) -> v4;
  auto dot( const v4&, const v4& ) -> float;
  auto Distance( const v4&, const v4& ) -> float;

} // namespace Tac


