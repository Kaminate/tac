#include <fstream>
#include "tacUtility.h"
#include "tacAlgorithm.h"
#include "tacPreprocessor.h"


TacSplitFilepath::TacSplitFilepath( const TacString& entireFilepath )
{
  auto found = entireFilepath.find_last_of( "/\\" );
  mDirectory = entireFilepath.substr( 0, found );
  mFilename = entireFilepath.substr( found + 1 );
}
TacString TacStripExt( const TacString& path )
{
  auto found = path.find_last_of( "." );
  TacAssert( found != TacString::npos );
  return path.substr( 0, found );
}
TacString TacStripLeadingSlashes( const TacString& path )
{
  int i;
  for( i = 0; i < path.size(); i++ )
    if( path[ i ] != '/' && path[ i ] != '\\' )
      break;
  TacString result = path.substr( i );
  return result;
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

bool TacStartsWith( const TacString& str, const TacString& prefix )
{
  if( str.size() < prefix.size() )
    return false;
  for( int i = 0; i < ( int )prefix.size(); ++i )
    if( prefix[ i ] != str[ i ] )
      return false;
  return true;
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

