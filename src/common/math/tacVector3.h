#pragma once

#include "src/common/math/tacVector2.h"

namespace Tac
{
  struct v3
  {
    v3() = default;
    v3( v2 xy, float z);
    v3( float xx, float yy, float zz );
    float*       begin();
    float*       end();
    float*       data();
    const float* data() const;
    v2&          xy();
    float&       operator[]( int );
    float        operator[]( int ) const;
    void         operator /= ( float );
    void         operator *= ( float );
    void         operator -= ( const v3& );
    void         operator += ( const v3& );
    bool         operator == ( const v3& ) const;
    bool         operator != ( const v3& ) const;
    v3           operator - () const;
    v3           operator * ( float ) const;
    v3           operator / ( float ) const;
    v3           operator + ( const v3& ) const;
    v3           operator - ( const v3& ) const;
    void         Normalize();
    float        Length() const;
    float        Quadrance() const;
    float        x;
    float        y;
    float        z;
  };

  v3             operator*( float , const v3& );
  float          Dot( const v3& , const v3& );
  v3             Cross( const v3& , const v3& );
  v3             Normalize( const v3& );
  float          Length( const v3& );
  float          Distance( const v3& , const v3& );
  float          Quadrance( const v3& );
  float          Quadrance( const v3& , const v3& );
  v3             Project( const v3& onto_b, const v3& of_a );
  void           GetFrameRH( const v3& normalizedDir, v3& unittan1, v3& unittan2 );
}
