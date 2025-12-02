#include "tac_string_util.h" // self-inc

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/os/tac_os.h"

bool Tac::IsLower( char c ) { return c >= 'a' && c <= 'z'; }
bool Tac::IsUpper( char c ) { return c >= 'A' && c <= 'Z'; }
char Tac::ToLower( char c ) { return IsUpper( c ) ? c + ( 'a' - 'A' ) : c; } // note: 'a' - 'A' == 32
char Tac::ToUpper( char c ) { return IsLower( c ) ? c - ( 'a' - 'A' ) : c; }

auto Tac::ToLower( const StringView str ) -> String
{
  String result;
  for( char c : str )
    result += ToLower( c );
  return result;
}

auto Tac::FormatPercentage( float num_0_to_1 ) -> String
{
  if( num_0_to_1 <= 0 )
    return "0%";

  if( num_0_to_1 >= 1 )
    return "100%";

  float t { num_0_to_1 * 100 };
  const int num_0_to_100 { (int)( num_0_to_1 * 100 ) };
  t -= ( int )t;
  t *= 100;

  String result;
  result += ToString( num_0_to_100 );
  result += ".";
  result += ToString( ( int )t );
  result += "%";
  return result;
}

auto Tac::FormatPercentage( float curr, float maxi ) -> String
{
  return Tac::FormatPercentage( curr / maxi );
}

bool Tac::IsAscii( const StringView s )
{
  for( char c : s )
  {
    auto u = static_cast< unsigned char >( c );
    if( u >= 128 )
      return false;
  }

  return true;
}

