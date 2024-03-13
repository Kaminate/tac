#pragma once
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector2.h"

namespace Tac
{
  struct v4
  {
    v4() = default;
    v4( float );
    v4( float , float , float , float  );
    v4( const v3& xyz, float w );
    v4( const v2& xy, float z, float w );
    float*       begin();
    float*       end();
    float*       data();
    const float* data() const;
    v3&          xyz();
    const v3&    xyz() const;
    float&       operator[]( int );
    float        operator[]( int ) const;
    void         operator /= ( float );
    void         operator *= ( float );
    void         operator -= ( const v4& );
    void         operator += ( const v4& );
    bool         operator == ( const v4& )const;
    bool         operator != ( const v4& )const;
    v4           operator - () const;
    v4           operator * ( float ) const;
    v4           operator / ( float ) const;
    v4           operator + ( const v4& ) const;
    v4           operator - ( const v4& ) const;
    void         Normalize();
    float        Length() const;
    float        Quadrance() const;
    float        x = 0;
    float        y = 0;
    float        z = 0;
    float        w = 0;
  };

  v4             operator*( float, const v4& );
  float          dot( const v4&, const v4& );
  float          Distance( const v4&, const v4& );

}


