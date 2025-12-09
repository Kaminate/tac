#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  struct v2;
  struct v2i
  {
    v2i() = default;
    v2i( int x, int y );
    auto begin() -> int*;
    auto end() -> int*;
    auto data() dynmc -> dynmc int*;
    auto data() const -> const int*;
    operator v2() const;
    auto operator[]( int ) dynmc -> dynmc int&;
    auto operator[]( int ) const -> const int&;
    void operator -= ( const v2i& );
    void operator += ( const v2i& );
    bool operator == ( const v2i& )const;
    bool operator != ( const v2i& )const;
    auto operator - () const -> v2i;
    auto operator + ( const v2i& ) const -> v2i;
    auto operator - ( const v2i& ) const -> v2i;

    int x {};
    int y {};
  };

  auto operator / ( v2i, int ) -> v2i;

} // namespace Tac

