#include <fstream>
//#include <cstring>
#include "tacUtility.h"
#include "tacAlgorithm.h"


TacSplitFilepath::TacSplitFilepath( const TacString& entireFilepath )
{
  auto found = entireFilepath.find_last_of( "/\\" );
  mDirectory = entireFilepath.substr( 0, found );
  mFilename = entireFilepath.substr( found + 1 );
}
void TacSaveToFile( const TacString& path, void* bytes, int byteCount, TacErrors& errors )
{
  std::ofstream ofs( path.c_str(), std::ofstream::binary );
  if( !ofs.is_open() )
  {
    errors = "Error: Failed to open file " + path + " for saving";
    return;
  }
  ofs.write( ( const char* )bytes, byteCount );
}
bool TacIsOfExt( const TacString& str, const TacString& ext )
{
  auto lower_str = TacToLower( str );
  auto lower_ext = TacToLower( ext );
  return TacEndsWith( lower_str, lower_ext );
}
bool TacFileExist( const TacString& str )
{
  return std::ifstream( str.c_str() ).good();
}

//
// String-manipulation
//
TacString TacSeparateStrings( const TacVector< TacString>& lines, const TacString& separator )
{
  TacString curSeparator;
  TacString result;
  for( const TacString& line : lines )
  {
    result += curSeparator;
    curSeparator = separator;
    result += line;
  }
  return result;
}
TacString TacSeparateNewline( const TacVector< TacString>& lines )
{
  return TacSeparateStrings( lines, "\n" );
}
TacString TacSeparateSpace( const TacVector< TacString>& lines )
{
  return TacSeparateStrings( lines, " " );
}
bool TacEndsWith( const TacString& str, const TacString& suffix )
{
  if( str.size() < suffix.size() )
    return false;
  for( int i = 0; i < ( int )suffix.size(); ++i )
    if( suffix[ suffix.size() - 1 - i ] != str[ str.size() - 1 - i ] )
      return false;
  return true;
}
TacString TacToLower( const TacString& str )
{
  TacString result;
  for( auto c : str )
    result += ( char )tolower( c );
  return result;
}
TacString TacFormatPercentage( float number_between_0_and_1 )
{
  TacString result;
  float t = number_between_0_and_1 * 100;
  result += TacToString( ( int )t );
  t -= ( int )t;
  t *= 100;
  result += ".";
  result += TacToString( ( int )t );
  result += "%";
  return result;
}
TacString TacFormatPercentage( float curr, float maxi )
{
  if( curr <= maxi )
    return "0%";
  return TacFormatPercentage( curr / maxi );
}

