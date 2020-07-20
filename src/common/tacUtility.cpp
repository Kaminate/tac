#include "src/common/tacUtility.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacPreprocessor.h"
#include <fstream>
namespace Tac
{



SplitFilepath::SplitFilepath( StringView entireFilepath )
{
  auto found = entireFilepath.find_last_of( "/\\" );
  mDirectory = entireFilepath.substr( 0, found );
  mFilename = entireFilepath.substr( found + 1 );
}
String StripExt( StringView path )
{
  auto found = path.find_last_of( "." );
  TAC_ASSERT( found != String::npos );
  return path.substr( 0, found );
}
String StripLeadingSlashes( StringView path )
{
  int i;
  for( i = 0; i < path.size(); i++ )
    if( path[ i ] != '/' && path[ i ] != '\\' )
      break;
  String result = path.substr( i );
  return result;
}
void SaveToFile( StringView path, void* bytes, int byteCount, Errors& errors )
{
  std::ofstream ofs( path.c_str(), std::ofstream::binary );
  if( !ofs.is_open() )
  {
    errors = "Error: Failed to open file " + path + " for saving";
    return;
  }
  ofs.write( ( const char* )bytes, byteCount );
}
bool IsOfExt( StringView str, StringView ext )
{
  auto lower_str = ToLower( str );
  auto lower_ext = ToLower( ext );
  return EndsWith( lower_str, lower_ext );
}
bool FileExist( StringView str )
{
  return std::ifstream( str.c_str() ).good();
}

//
// String-manipulation
//
String SeparateStrings( const Vector< String>& lines, StringView separator )
{
  String curSeparator;
  String result;
  for( StringView line : lines )
  {
    result += curSeparator;
    curSeparator = separator;
    result += line;
  }
  return result;
}
String SeparateNewline( const Vector< String>& lines )
{
  return SeparateStrings( lines, "\n" );
}
String SeparateSpace( const Vector< String>& lines )
{
  return SeparateStrings( lines, " " );
}

bool StartsWith( StringView str, StringView prefix )
{
  if( str.size() < prefix.size() )
    return false;
  for( int i = 0; i < ( int )prefix.size(); ++i )
    if( prefix[ i ] != str[ i ] )
      return false;
  return true;
}
bool EndsWith( StringView str, StringView suffix )
{
  if( str.size() < suffix.size() )
    return false;
  for( int i = 0; i < ( int )suffix.size(); ++i )
    if( suffix[ suffix.size() - 1 - i ] != str[ str.size() - 1 - i ] )
      return false;
  return true;
}
String ToLower( StringView str )
{
  String result;
  for( auto c : str )
    result += ( char )tolower( c );
  return result;
}
String FormatPercentage( float number_between_0_and_1 )
{
  String result;
  float t = number_between_0_and_1 * 100;
  result += ToString( ( int )t );
  t -= ( int )t;
  t *= 100;
  result += ".";
  result += ToString( ( int )t );
  result += "%";
  return result;
}
String FormatPercentage( float curr, float maxi )
{
  if( curr <= maxi )
    return "0%";
  return FormatPercentage( curr / maxi );
}

}
