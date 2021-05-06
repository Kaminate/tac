#include "src/common/tacTemporaryMemory.h"
#include <fstream>

namespace Tac
{
  TemporaryMemory TemporaryMemoryFromFile( StringView path, Errors& errors )
  {
    std::ifstream ifs( path, std::ifstream::binary );
    TAC_HANDLE_ERROR_IF_REUTRN( !ifs.is_open(), "Error: Failed to open file " + String( path ) +
                                " while allocating temporary memory", errors, {} );
    //if( !ifs.is_open() )
    //{
    //  const String errorMsg = "Error: Failed to open file " + String( path ) + " while allocating temporary memory";
    //  errors.Append( errorMsg );
    //  errors.Append( TAC_STACK_FRAME );
    //  return {};
    //}
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

  void WriteToFile( StringView path, void* bytes, int byteCount, Errors& errors )
  {
    std::ofstream ofs( path.c_str(), std::ofstream::binary );
    if( !ofs.is_open() )
    {
      const String errMsg =  "Failed to open file " + String( path ) + " for saving";
      TAC_RAISE_ERROR( errMsg, errors );
    }
    ofs.write( ( const char* )bytes, ( std::streamsize ) byteCount );
  }
  String FileToString( StringView path, Errors& errors )
  {
    TemporaryMemory temporaryMemory = TemporaryMemoryFromFile( path, errors );
    if( errors )
      return "";
    String result( temporaryMemory.data(), ( int )temporaryMemory.size() );
    return result;
  }
}
