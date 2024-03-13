#include "tac-std-lib/math/tac_vector3i.h"
#include "tac-std-lib/math/tac_math.h"

namespace Tac
{
  v3i::v3i( int xx, int yy, int zz ) : x( xx ), y( yy ), z( zz ){}
  v3i::v3i( int xyz ) : x( xyz ), y( xyz ), z( xyz ){}
  v3i::v3i( v2i xy, int zz ) : x( xy.x ), y( xy.y ), z( zz ){}
  v2i&       v3i::xy() { return *( v2i* )data(); }
  int*       v3i::begin() { return data(); }
  int*       v3i::end() { return data() + 3; }
  int*       v3i::data() { return &x; }
  const int* v3i::data() const{ return &x; }
  int&       v3i::operator[]( int i ) { return data()[ i ]; }
  int        v3i::operator[]( int i ) const { return data()[ i ]; }
  void       v3i::operator -= ( const v3i& v ) { for( int i = 0; i < 3; ++i ) data()[ i ] -= v[ i ]; }
  void       v3i::operator += ( const v3i& v ) { for( int i = 0; i < 3; ++i ) data()[ i ] += v[ i ]; }
  bool       v3i::operator == ( const v3i& v )const
  {
    for( int i = 0; i < 3; ++i )
      if( data()[ i ] != v[ i ] )
        return false;
    return true;
  }
  bool       v3i::operator != ( const v3i& v )const
  {
    for( int i = 0; i < 3; ++i )
      if( data()[ i ] != v[ i ] )
        return true;
    return false;
  }
  v3i        v3i::operator - () const { return { -x, -y, -z }; }
  v3i        v3i::operator + ( const v3i& v ) const { v3i result = *this; result += v; return result; }
  v3i        v3i::operator - ( const v3i& v ) const { v3i result = *this; result -= v; return result; }
}
