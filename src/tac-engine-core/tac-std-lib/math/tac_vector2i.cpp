#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector2.h"

namespace Tac
{
  v2i::v2i( int xx, int yy ) : x( xx ), y( yy )      {}
  v2i::operator v2() const                           { return { ( float )x, ( float )y }; }
  auto v2i::begin() -> int*                          { return data(); }
  auto v2i::end() -> int*                            { return data() + 2; }
  auto v2i::data() dynmc -> dynmc int*               { return &x; }
  auto v2i::data() const -> const int*               { return &x; }
  auto v2i::operator[]( int i ) const -> const int&  { return data()[ i ]; }
  auto v2i::operator[]( int i ) dynmc -> dynmc int&  { return data()[ i ]; }
  void v2i::operator -= ( const v2i& v )             { for( int i{}; i < 2; ++i ) data()[ i ] -= v[ i ]; }
  void v2i::operator += ( const v2i& v )             { for( int i{}; i < 2; ++i ) data()[ i ] += v[ i ]; }
  bool v2i::operator == ( const v2i& v ) const       { return x == v.x && y == v.y; }
  bool v2i::operator != ( const v2i& v ) const       { return x != v.x || y != v.y; }
  auto v2i::operator - () const -> v2i               { return { -x,-y }; }
  auto v2i::operator + ( const v2i& v ) const -> v2i { v2i result = *this; result += v; return result; }
  auto v2i::operator - ( const v2i& v ) const -> v2i { v2i result = *this; result -= v; return result; }
}

auto Tac::operator / ( const v2i v, int i ) -> v2i { return { v.x / i, v.y / i }; }

