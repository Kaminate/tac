#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector2.h"

namespace Tac
{
  v2i::v2i( int xx, int yy ) : x( xx ), y( yy ) {}
  v2i::operator v2() const { return { (float)x,(float)y }; }
  int*       v2i::begin()      { return data(); }
  int*       v2i::end()        { return data() + 2; }
  int*       v2i::data()       { return &x; }
  const int* v2i::data() const { return &x; }
  int&       v2i::operator[]( int i ) { return data()[ i ]; }
  int        v2i::operator[]( int i ) const { return data()[ i ]; }
  void       v2i::operator -= ( const v2i& v ) { for( int i = 0; i < 2; ++i ) data()[ i ] -= v[ i ]; }
  void       v2i::operator += ( const v2i& v ) { for( int i = 0; i < 2; ++i ) data()[ i ] += v[ i ]; }
  bool       v2i::operator == ( const v2i& v ) const
  {
    for( int i = 0; i < 2; ++i )
      if( data()[ i ] != v[ i ] )
        return false;
    return true;
  }
  bool       v2i::operator != ( const v2i& v ) const
  {
    for( int i = 0; i < 2; ++i )
      if( data()[ i ] != v[ i ] )
        return true;
    return false;
  }
  v2i        v2i::operator - () const { return { -x,-y }; }
  v2i        v2i::operator + ( const v2i& v ) const { v2i result = *this; result += v; return result; }
  v2i        v2i::operator - ( const v2i& v ) const { v2i result = *this; result -= v; return result; }


}
