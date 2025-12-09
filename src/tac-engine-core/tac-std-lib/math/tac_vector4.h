#pragma once
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

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
    auto data() dynmc -> dynmc float*;
    auto data() const -> const float*;
    auto xy() dynmc -> dynmc v2&;
    auto xy() const -> const v2&;
    auto xyz() dynmc -> dynmc v3&;
    auto xyz() const -> const v3&;
    void Normalize();
    auto Length() const -> float;
    auto Quadrance() const -> float;
    auto operator[]( int ) dynmc -> dynmc float&;
    auto operator[]( int ) const -> const float&;
    void operator /= ( float );
    void operator *= ( float );
    void operator -= ( const v4& );
    void operator += ( const v4& );
    bool operator == ( const v4& ) const;
    bool operator != ( const v4& ) const;
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


