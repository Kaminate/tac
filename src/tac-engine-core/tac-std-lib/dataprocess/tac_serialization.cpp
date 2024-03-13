#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h" // assert
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"

namespace Tac
{
  const NetworkBit& NetworkBits::operator[]( int i ) const { return mNetworkBits[ i ]; }
  int               NetworkBits::size() const              { return mNetworkBits.size(); }
  void              NetworkBits::Add( NetworkBit b )       { mNetworkBits.push_back( b ); }

  Endianness GetEndianness()
  {
    union U16B
    {
      u16 mu16;
      u8 mu8[ 2 ];
    };

    U16B u16b;
    u16b.mu16 = 0xABCD;

    return ( u16b.mu8[ 0 ] == 0xCD && u16b.mu8[ 1 ] == 0xAB )
      ? Endianness::Little
      : Endianness::Big;
  }

  static void CopyValueAccountForEndianness( void* dst,
                                             const void* src,
                                             int size,
                                             Endianness a,
                                             Endianness b )
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
    auto srcDiff = ( std::intptr_t )valueCount * ( intptr_t )sizeOfValue;
    if( dstDiff < srcDiff )
      return false;
    for( int i = 0; i < valueCount; ++i )
      CopyValueAccountForEndianness( ( char* )values + ( intptr_t )sizeOfValue * i,
      ( char* )mBegin + ( intptr_t )sizeOfValue * i,
                                     sizeOfValue, mFrom, mTo );
    mBegin = ( char* )mBegin + ( intptr_t )sizeOfValue * valueCount;
    return true;
  }
  bool Reader::Read( void* bytes, const NetworkBits& networkBits )
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
    {
      void* dst = ( char* )mBytes.data() + std::ptrdiff_t( oldSize + i * sizeOfValue);
      const void* src = ( const char* )values + sizeOfValue * i;
      CopyValueAccountForEndianness( dst, src, sizeOfValue, mFrom, mTo );
    }
  }
  void Writer::Write( const void* bytes,
                      NetBitDiff netBitDiff,
                      const NetworkBits& networkBits )
  {
    const u8 bitfield = netBitDiff.mBitfield;
    TAC_ASSERT( bitfield );

    Write( bitfield );
    for( int networkBitIndex = 0; networkBitIndex < networkBits.size(); ++networkBitIndex )
    {
      if( !netBitDiff.IsSet( networkBitIndex ) )
        continue;

      const NetworkBit& networkBit = networkBits[ networkBitIndex ];
      Write( ( char* )bytes + networkBit.mByteOffset,
             networkBit.mComponentCount,
             networkBit.mComponentByteCount );
    }
  }
} // namespace Tac

