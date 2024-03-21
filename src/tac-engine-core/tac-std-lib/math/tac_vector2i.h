#pragma once

namespace Tac
{
  struct v2;
  struct v2i
  {
    int x = 0;
    int y = 0;
    int*       begin();
    int*       end();
    int*       data();
    const int* data() const;
    v2i() = default;
    v2i( int x, int y );
    operator v2() const;
    int& operator[]( int );
    int  operator[]( int ) const;
    void operator -= ( const v2i& );
    void operator += ( const v2i& );
    bool operator == ( const v2i& )const;
    bool operator != ( const v2i& )const;
    v2i  operator - () const;
    v2i  operator + ( const v2i& ) const;
    v2i  operator - ( const v2i& ) const;
  };

}
