#include "tacMemory.h"

#include <fstream>

TacTemporaryMemory TacTemporaryMemoryFromFile( const TacStringView& path, TacErrors& errors )
{
  std::ifstream ifs( path.c_str(), std::ifstream::binary );
  if( !ifs.is_open() )
  {
    errors = "Error: Failed to open file " + path + " while allocating temporary memory";
    return {};
  }
  ifs.seekg( 0, std::ifstream::end );
  auto byteCount = ifs.tellg();
  ifs.seekg( 0, std::ifstream::beg );
  TacTemporaryMemory result( ( int )byteCount );
  ifs.read( result.data(), byteCount );
  return result;
}
TacTemporaryMemory TacTemporaryMemoryFromBytes( const void* bytes, int byteCount )
{
  TacTemporaryMemory result( byteCount );
  TacMemCpy( result.data(), bytes, byteCount );
  return result;
}

void TacWriteToFile( const TacString& path, void* bytes, int byteCount, TacErrors& errors )
{
  std::ofstream ofs( path.c_str(), std::ofstream::binary );
  if( !ofs.is_open() )
  {
    errors = "Failed to open file " + path + " for saving";
    return;
  }
  ofs.write( ( const char* )bytes, ( std::streamsize   ) byteCount );
}
