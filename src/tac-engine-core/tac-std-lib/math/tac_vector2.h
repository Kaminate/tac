#pragma once

namespace Tac
{
  struct v2i;
  struct v2
  {
    float x = 0;
    float y = 0;
    float*       begin();
    float*       end();
    float*       data();
    const float* data() const;
    v2() = default;
    v2( float x, float y );
    operator v2i() const;
    float& operator[]( int );
    float  operator[]( int ) const;
    void   operator /= ( float );
    void   operator *= ( float );
    void   operator -= ( const v2& );
    void   operator += ( const v2& );
    bool   operator == ( const v2& )const;
    bool   operator != ( const v2& )const;
    v2     operator - () const;
    v2     operator * ( float ) const;
    v2     operator / ( float ) const;
    v2     operator + ( const v2& ) const;
    v2     operator - ( const v2& ) const;
    void   Normalize();
    float  Length() const;
    float  Quadrance() const;
  };

  v2 operator *( float, const v2& );
  float Dot( const v2&, const v2& );
  v2    Normalize( const v2& );
  float Length( const v2& );
  float Distance( const v2&, const v2& );
  float Quadrance( const v2& );
  float Quadrance( const v2&, const v2& );
}
