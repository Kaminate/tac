#include <algorithm> // std::reverse
#include <cstring> // std::memcpy
#include "common/tacSerialization.h"
#include "common/tacPreprocessor.h" // assert

TacEndianness TacGetEndianness()
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
    return TacEndianness::Little;
  return TacEndianness::Big;
}

bool TacReader::Read( void* values, int valueCount, int sizeOfValue )
{
  TacAssert( mFrom != TacEndianness::Unknown );
  TacAssert( mTo != TacEndianness::Unknown );
  if( ( char* )mEnd - ( char* )mBegin <  valueCount * sizeOfValue )
    return false;
  auto value = ( char* )values;
  for( int i = 0; i < valueCount; ++i )
  {
    std::memcpy( value, mBegin, sizeOfValue );
    if( mFrom != mTo )
      std::reverse( value, value + sizeOfValue );
    value += sizeOfValue;
    mBegin = ( char* )mBegin + sizeOfValue;
  }
  return true;
}
bool TacReader::Read( void* bytes, const TacVector< TacNetworkBit >& networkBits )
{
  uint8_t bitfield;
  if( !Read( &bitfield ) )
    return false;
  TacAssert( bitfield ); // y u sending me nothin
  for( int networkBitIndex = 0; networkBitIndex < networkBits.size(); ++networkBitIndex )
  {
    if( !( bitfield & ( 1 << networkBitIndex ) ) )
      continue;
    auto networkBit = networkBits[ networkBitIndex ];
    if( !Read(
      ( char* )bytes + networkBit.mByteOffset,
      networkBit.mComponentCount, // TODO: Is this the right variable? SHould it be COmponentByteCount?
      networkBit.mComponentByteCount ) )
      return false;
  }
  return true;
}

void TacWriter::Write( const void* values, int valueCount, int sizeOfValue )
{
  TacAssert( mFrom != TacEndianness::Unknown );
  TacAssert( mTo != TacEndianness::Unknown );
  auto value = ( const char* )values;
  for( int i = 0; i < valueCount; ++i )
  {
    mBytes.resize( mBytes.size() + sizeOfValue );
    auto destination = ( char* )mBytes.data();
    std::memcpy( destination, value, sizeOfValue );
    if( mFrom != mTo )
      std::reverse( ( char* )destination, ( char* )destination + sizeOfValue );
    value += sizeOfValue;
  }
}
void TacWriter::Write(
  const void* bytes,
  uint8_t bitfield,
  const TacVector< TacNetworkBit >& networkBits )
{
  Write( bitfield );
  TacAssert( bitfield );
  for( int networkBitIndex = 0; networkBitIndex < networkBits.size(); ++networkBitIndex )
  {
    if( !( bitfield & ( 1 << networkBitIndex ) ) )
      continue;
    const TacNetworkBit& networkBit = networkBits[ networkBitIndex ];
    Write(
      ( char* )bytes + networkBit.mByteOffset,
      networkBit.mComponentCount,
      networkBit.mComponentByteCount );
  }
}
