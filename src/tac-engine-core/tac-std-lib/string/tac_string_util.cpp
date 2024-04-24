#include "tac_string_util.h" // self-inc

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static Vector< String > GetLines( StringView str )
  {
    Vector< String > lines;

    String line;
    for( char c : str )
    {
      if( c == '\n' )
      {
        lines.push_back( line );
        line.clear();
      }
      else
      {
        line += c;
      }
    }
    lines.push_back( line );
    return lines;
  }

  static int LongestLineSize( const Vector<String>& lines )
  {
    int longest { 0 };
    for( const String& str : lines )
      longest = Max( longest, str.size() );
    return longest;
  }
}

// Note: 'A' == 65, 'a' == 97
bool   Tac::IsLower( char c ) { return c >= 'a' && c <= 'z'; }
bool   Tac::IsUpper( char c ) { return c >= 'A' && c <= 'Z'; }
char   Tac::ToLower( char c ) { return IsUpper( c ) ? c + ( 'a' - 'A' ) : c; }
char   Tac::ToUpper( char c ) { return IsLower( c ) ? c - ( 'a' - 'A' ) : c; }

Tac::String Tac::ToLower( const StringView& str )
{
  String result;
  for( char c : str )
    result += ToLower( c );
  return result;
}

Tac::String Tac::FormatPercentage( float num_0_to_1 )
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

Tac::String Tac::FormatPercentage( float curr, float maxi )
{
  return Tac::FormatPercentage( curr / maxi );
}

bool Tac::IsAscii( const StringView& s )
{
  for( char c : s )
  {
    auto u = static_cast< unsigned char >( c );
    if( u >= 128 )
      return false;
  }

  return true;
}


Tac::String Tac::AsciiBoxAround( const StringView& str )
{
  if( str.empty() )
  {
    return
      "+-----+\n"
      "| n/a |\n"
      "+-----+";
  }

  const Vector< String > lines { GetLines( str ) };
  const int n { LongestLineSize( lines ) };


  const String topbot { String() + "+-" + String( n, '-' ) + "-+" };

  String result;
  result += topbot + '\n';
  for( const String& line : lines )
  {
    result += "| " + line + String( n - line.size(), ' ' ) + " |" + "\n";
  }
  result += topbot;
  return result;
}
