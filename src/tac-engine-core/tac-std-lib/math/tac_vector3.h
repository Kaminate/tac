#pragma once

#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  struct v3
  {
    v3() = default;
    explicit v3( float );
    v3( v2, float );
    v3( float, float, float );
    auto begin() -> float*;
    auto end() -> float*;
    auto data() dynmc -> dynmc float*;
    auto data() const -> const float*;
    auto xy() const -> const v2&;
    auto xy() dynmc -> dynmc v2&;
    void Normalize();
    auto Length() const -> float;
    auto Quadrance() const -> float;
    auto operator[]( int ) const -> const float&;
    auto operator[]( int ) dynmc -> dynmc float&;
    void operator /= ( float );
    void operator *= ( float );
    void operator -= ( const v3& );
    void operator += ( const v3& );
    bool operator == ( const v3& ) const;
    bool operator != ( const v3& ) const;
    auto operator - () const -> v3;
    auto operator * ( float ) const -> v3;
    auto operator / ( float ) const -> v3;
    auto operator + ( const v3& ) const -> v3;
    auto operator - ( const v3& ) const -> v3;
    float x {};
    float y {};
    float z {};
  };

  auto operator*( float, const v3& ) -> v3;
  auto Dot( const v3&, const v3& ) -> float;
  auto Cross( const v3&, const v3& ) -> v3;
  auto Normalize( const v3& ) -> v3;
  auto Length( const v3& ) -> float;
  auto Distance( const v3&, const v3& ) -> float;
  auto Quadrance( const v3& ) -> float;
  auto Quadrance( const v3&, const v3& ) -> float;


  // Given two vectors, a and b, Project(b, a) returns the projection of a onto b,
  // in the direction (parallel) of b.
  auto Project( const v3& onto_b, const v3& of_a ) -> v3;

  void GetFrameRH( const v3& normalizedDir, v3& unittan1, v3& unittan2 );

  void v3UnitTest();

} // namespace Tac
