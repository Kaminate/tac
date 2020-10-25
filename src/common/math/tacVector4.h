#pragma once
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacVector2.h"

namespace Tac
{
  struct v4
  {
    v4() = default;
    v4( float xx, float yy, float zz, float ww );
    v4( const v3& xyz, float ww );
    v4( const v2& xy, float zz, float ww );
    float*       begin();
    float*       end();
    float*       data();
    const float* data() const;
    v3&          xyz();
    const v3&    xyz() const;
    float&       operator[]( int i );
    float        operator[]( int i ) const;
    void         operator /= ( float v );
    void         operator *= ( float v );
    void         operator -= ( const v4& v );
    void         operator += ( const v4& v );
    bool         operator == ( const v4& v )const;
    bool         operator != ( const v4& v )const;
    v4           operator - () const;
    v4           operator * ( float v ) const;
    v4           operator / ( float v ) const;
    v4           operator + ( const v4& v ) const;
    v4           operator - ( const v4& v ) const;
    void         Normalize();
    float        Length() const;
    float        Quadrance() const;
    float        x;
    float        y;
    float        z;
    float        w;
  };

  v4             operator*( float f, const v4& v );
  float          dot( const v4& lhs, const v4& rhs );
  float          Distance( const v4& lhs, const v4& rhs );

}
