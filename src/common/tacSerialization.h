// This file is used to shove data in and out of buffers
// ( which may be files or network streams )

#pragma once

#include "src/common/containers/tacVector.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacVector4.h"

#include <cstdint>

namespace Tac
{


  enum class Endianness
  {
    Unknown,
    Little, // Common on most intel cpu architectures
    Big, // Also called host byte order
  };

  Endianness GetEndianness();

  struct NetworkBit
  {
    const char* mDebugName;
    int         mByteOffset;
    int         mComponentByteCount; // rename sizeOfComponent
    int         mComponentCount;
  };

  struct Reader
  {
    bool Read( void* values, int valueCount, int sizeOfValue );
    bool Read( void* bytes, const Vector< NetworkBit >& networkBits );
    template< typename T > bool Read( T* values, int valueCount = 1 ) { return Read( values, valueCount, sizeof( T ) ); }
    Endianness  mFrom = Endianness::Unknown;
    Endianness  mTo = Endianness::Unknown;
    const void* mBegin = nullptr;
    const void* mEnd = nullptr;
  };

  template<> inline bool Reader::Read( v2* values, int valueCount ) { return Read( values->data(), 2 * valueCount, sizeof( float ) ); }
  template<> inline bool Reader::Read( v3* values, int valueCount ) { return Read( values->data(), 3 * valueCount, sizeof( float ) ); }
  template<> inline bool Reader::Read( v4* values, int valueCount ) { return Read( values->data(), 4 * valueCount, sizeof( float ) ); }

  struct Writer
  {
    void Write( const void* values, int valueCount, int sizeOfValue );
    void Write( const void* bytes, uint8_t bitfield, const Vector< NetworkBit >& networkBits );
    template< typename T > void Write( T t )                    { return Write( &t, 1, sizeof( T ) ); }
    template< typename T > void Write( const T* t, int tCount ) { return Write( t, tCount, sizeof( T ) ); }
    Endianness     mFrom = Endianness::Unknown;
    Endianness     mTo = Endianness::Unknown;
    Vector< char > mBytes;
  };
  template<> inline void Writer::Write( const v2& t ) { return Write( t.data(), 2, sizeof( float ) ); }
  template<> inline void Writer::Write( const v3& t ) { return Write( t.data(), 3, sizeof( float ) ); }
  template<> inline void Writer::Write( const v4& t ) { return Write( t.data(), 4, sizeof( float ) ); }

}
