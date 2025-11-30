#pragma once
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  struct v2i;
  struct v2
  {
    float x {};
    float y {};
    v2() = default;
    v2( float x, float y );
    auto begin() -> float*;
    auto end() -> float*;
    auto data() dynmc -> dynmc float*;
    auto data() const -> const float*;
    void Normalize();
    auto Length() const -> float;
    auto Quadrance() const -> float;
    operator v2i() const;
    auto operator[]( int ) dynmc -> dynmc float&;
    auto operator[]( int ) const -> const float&;
    void operator /= ( float );
    void operator *= ( float );
    void operator -= ( const v2& );
    void operator += ( const v2& );
    bool operator == ( const v2& )const;
    bool operator != ( const v2& )const;
    auto operator - () const -> v2;
    auto operator * ( float ) const -> v2;
    auto operator / ( float ) const -> v2;
    auto operator / ( int ) const -> v2;
    auto operator + ( const v2& ) const -> v2;
    auto operator - ( const v2& ) const -> v2;
  };

  auto operator *( float, const v2& ) -> v2;
  auto Dot( const v2&, const v2& ) -> float;
  auto Normalize( const v2& ) -> v2;
  auto Length( const v2& ) -> float;
  auto Distance( const v2&, const v2& ) -> float;
  auto Quadrance( const v2& ) -> float;
  auto Quadrance( const v2&, const v2& ) -> float;
}
