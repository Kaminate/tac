#pragma once

namespace Tac
{
  struct v2;
  struct v2i
  {
    v2i() = default;
    v2i( int x, int y );
    int*       begin();
    int*       end();
    int*       data();
    const int* data() const;
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

    int x {};
    int y {};
  };

  v2i operator / ( const v2i, int );

} // namespace Tac

