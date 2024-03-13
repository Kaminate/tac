#pragma once

#include "tac-std-lib/math/tac_vector2i.h"

namespace Tac
{
  struct v3i
  {
    v3i() = default;
    v3i( int );
    v3i( v2i, int );
    v3i( int, int, int );
    int*       begin();
    int*       end();
    int*       data();
    const int* data() const;
    v2i&        xy();
    int&       operator[]( int );
    int        operator[]( int ) const;
    void       operator -= ( const v3i& );
    void       operator += ( const v3i& );
    bool       operator == ( const v3i& ) const;
    bool       operator != ( const v3i& ) const;
    v3i        operator - () const;
    v3i        operator + ( const v3i& ) const;
    v3i        operator - ( const v3i& ) const;
    int        x;
    int        y;
    int        z;
  };

} // namespace Tac
