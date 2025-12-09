#include "tac-std-lib/math/tac_vector3i.h"
#include "tac-std-lib/math/tac_math.h"

namespace Tac
{
  v3i::v3i( int xx, int yy, int zz ) : x( xx ), y( yy ), z( zz ){}
  v3i::v3i( int xyz ) : x( xyz ), y( xyz ), z( xyz ){}
  v3i::v3i( v2i xy, int zz ) : x( xy.x ), y( xy.y ), z( zz ){}
  auto v3i::xy() const -> const v2i&                 { return *( v2i* )data(); }
  auto v3i::xy() dynmc-> dynmc v2i&                  { return *( v2i* )data(); }
  auto v3i::begin() -> int*                          { return data(); }
  auto v3i::end() -> int*                            { return data() + 3; }
  auto v3i::data() dynmc -> dynmc int*               { return &x; }
  auto v3i::data() const-> const int*                { return &x; }
  auto v3i::operator[]( int i ) const -> const int&  { return data()[ i ]; }
  auto v3i::operator[]( int i ) dynmc -> dynmc int&  { return data()[ i ]; }
  void v3i::operator -= ( const v3i& v )             { for( int i{}; i < 3; ++i ) data()[ i ] -= v[ i ]; }
  void v3i::operator += ( const v3i & v )            { for( int i{}; i < 3; ++i ) data()[ i ] += v[ i ]; }
  bool v3i::operator == ( const v3i& v ) const       { return x == v.x && y == v.y && z == v.z; }
  bool v3i::operator != ( const v3i& v ) const       { return x != v.x || y != v.y || z != v.z; }
  auto v3i::operator - () const -> v3i               { return { -x, -y, -z }; }
  auto v3i::operator + ( const v3i& v ) const -> v3i { v3i result = *this; result += v; return result; }
  auto v3i::operator - ( const v3i& v ) const -> v3i { v3i result = *this; result -= v; return result; }
}
