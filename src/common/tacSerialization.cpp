#include "src/common/tacSerialization.h"
#include "src/common/tacPreprocessor.h" // assert
#include "src/common/tacString.h"
#include "src/common/tacAlgorithm.h"

#include <cinttypes>


namespace Tac
{


  Endianness GetEndianness()
  {
    union U16B
    {
      uint16_t mu16;
      uint8_t mu8[ 2 ];
    };
    U16B u16b;
    u16b.mu16 = 0xABCD;
    if( u16b.mu8[ 0 ] == 0xCD &&
        u16b.mu8[ 1 ] == 0xAB )
      return Endianness::Little;
    return Endianness::Big;
  }

  static void CopyValueAccountForEndianness( void* dst, const void* src, int size, Endianness a, Endianness b )
  {
    MemCpy( dst, src, size );
    if( a != b )
      Reverse( ( char* )dst, ( char* )dst + size );
  }

  bool Reader::Read( void* values, int valueCount, int sizeOfValue )
  {
    TAC_ASSERT( mFrom != Endianness::Unknown );
    TAC_ASSERT( mTo != Endianness::Unknown );
    auto dstDiff = ( char* )mEnd - ( char* )mBegin;
    auto srcDiff = ( intptr_t )valueCount * ( intptr_t )sizeOfValue;
    if( dstDiff < srcDiff )
      return false;
    for( int i = 0; i < valueCount; ++i )
      CopyValueAccountForEndianness( ( char* )values + (intptr_t)sizeOfValue * i,
                                     ( char* )mBegin + (intptr_t)sizeOfValue * i,
                                     sizeOfValue, mFrom, mTo );
    mBegin = ( char* )mBegin + (intptr_t)sizeOfValue * valueCount;
    return true;
  }
  bool Reader::Read( void* bytes, const Vector< NetworkBit >& networkBits )
  {
    char bitfield;
    if( !Read( &bitfield ) )
      return false;
    TAC_ASSERT( bitfield ); // y u sending me nothin
    for( int networkBitIndex = 0; networkBitIndex < networkBits.size(); ++networkBitIndex )
    {
      if( !( bitfield & ( 1 << networkBitIndex ) ) )
        continue;
      auto networkBit = networkBits[ networkBitIndex ];
      if( !Read( ( char* )bytes + networkBit.mByteOffset,
                 networkBit.mComponentCount,
                 networkBit.mComponentByteCount ) )
        return false;
    }
    return true;
  }

  void Writer::Write( const void* values,
                      int valueCount,
                      int sizeOfValue )
  {
    TAC_ASSERT( mFrom != Endianness::Unknown );
    TAC_ASSERT( mTo != Endianness::Unknown );
    const int oldSize = mBytes.size();
    mBytes.resize( oldSize + sizeOfValue * valueCount );
    for( int i = 0; i < valueCount; ++i )
      CopyValueAccountForEndianness( ( char* )mBytes.data() + oldSize + i * sizeOfValue,
                                     ( const char* )values + sizeOfValue * i,
                                     sizeOfValue, mFrom, mTo );
  }
  void Writer::Write( const void* bytes,
                      char bitfield,
                      const Vector< NetworkBit >& networkBits )
  {
    Write( bitfield );
    TAC_ASSERT( bitfield );
    for( int networkBitIndex = 0; networkBitIndex < networkBits.size(); ++networkBitIndex )
    {
      if( !( bitfield & ( 1 << networkBitIndex ) ) )
        continue;
      const NetworkBit& networkBit = networkBits[ networkBitIndex ];
      Write( ( char* )bytes + networkBit.mByteOffset,
             networkBit.mComponentCount,
             networkBit.mComponentByteCount );
    }
  }
}
