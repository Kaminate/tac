#pragma once

#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  struct v3i
  {
    v3i() = default;
    v3i( int );
    v3i( v2i, int );
    v3i( int, int, int );
    auto begin() -> int*;
    auto end() -> int*;
    auto data() dynmc -> dynmc int*;
    auto data() const -> const int*;
    auto xy() const -> const v2i&;
    auto xy() dynmc -> dynmc v2i&;
    auto operator[]( int ) dynmc -> dynmc int&;
    auto operator[]( int ) const -> const int&;
    void operator -= ( const v3i& );
    void operator += ( const v3i& );
    bool operator == ( const v3i& ) const;
    bool operator != ( const v3i& ) const;
    auto operator - () const -> v3i;
    auto operator + ( const v3i& ) const -> v3i;
    auto operator - ( const v3i& ) const -> v3i;

    int x {};
    int y {};
    int z {};
  };

} // namespace Tac
