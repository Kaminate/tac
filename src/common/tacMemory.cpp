#include "src/common/tacMemory.h"

#include <fstream>

namespace Tac
{


  TemporaryMemory TemporaryMemoryFromFile( const StringView& path, Errors& errors )
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
    TemporaryMemory result( ( int )byteCount );
    ifs.read( result.data(), byteCount );
    return result;
  }
  TemporaryMemory TemporaryMemoryFromBytes( const void* bytes, int byteCount )
  {
    TemporaryMemory result( byteCount );
    MemCpy( result.data(), bytes, byteCount );
    return result;
  }

  void WriteToFile( const String& path, void* bytes, int byteCount, Errors& errors )
  {
    std::ofstream ofs( path.c_str(), std::ofstream::binary );
    if( !ofs.is_open() )
    {
      errors = "Failed to open file " + path + " for saving";
      return;
    }
    ofs.write( ( const char* )bytes, ( std::streamsize ) byteCount );
  }
  String FileToString( const StringView& path, Errors& errors )
  {
    TemporaryMemory temporaryMemory = TemporaryMemoryFromFile( path, errors );
    if( errors )
      return "";
    String result( temporaryMemory.data(), ( int )temporaryMemory.size() );
    return result;
  }

  static thread_local StackFrame sNewStackFrame;
  void SetNewStackFrame( StackFrame stackFrame )
  {
    sNewStackFrame = stackFrame;
  }
}

void operator delete( void* ptr ) noexcept
{
  std::free( ptr );
}

void* operator new( std::size_t sz )
//void* operator new( std::size_t sz, Tac::StackFrame stackFrame )
{
  Tac::sNewStackFrame;
  void* result = std::malloc( sz );
  return result;
}
