// This file is used to shove data in and out of buffers
// ( which may be files or network streams )

#pragma once

#include "common/containers/tacVector.h"
#include "common/math/tacVector2.h"
#include "common/math/tacVector3.h"
#include "common/math/tacVector4.h"

#include <cstdint>

enum class TacEndianness
{
  Unknown,
  Little, // Common on most intel cpu architectures
  Big, // Also called host byte order
};

TacEndianness TacGetEndianness();

struct TacNetworkBit
{
  const char* mDebugName;
  int mByteOffset;
  int mComponentByteCount; // rename sizeOfComponent
  int mComponentCount;
};

struct TacReader
{
  bool Read( void* values, int valueCount, int sizeOfValue );
  template< typename T > bool Read( T* values, int valueCount = 1 ){
    return Read( values, valueCount, sizeof( T ) ); }
  bool Read( void* bytes, const TacVector< TacNetworkBit >& networkBits );
  TacEndianness mFrom = TacEndianness::Unknown;
  TacEndianness mTo = TacEndianness::Unknown;
  void* mBegin = nullptr;
  void* mEnd = nullptr;
};
template<> inline bool TacReader::Read( v2* values, int valueCount ){
  return Read( values->data(), 2 * valueCount, sizeof( float ) ); }
template<> inline bool TacReader::Read( v3* values, int valueCount ){
  return Read( values->data(), 3 * valueCount, sizeof( float ) ); }
template<> inline bool TacReader::Read( v4* values, int valueCount ){
  return Read( values->data(), 4 * valueCount, sizeof( float ) ); }

struct TacWriter
{
  void Write( const void* values, int valueCount, int sizeOfValue );
  template< typename T > void Write( T t ){ return Write( &t, 1, sizeof( T ) ); }
  template< typename T > void Write( const T* t, int tCount ){ return Write( t, tCount, sizeof( T ) ); }
  void Write(
    const void* bytes,
    uint8_t bitfield,
    const TacVector< TacNetworkBit >& networkBits );
  TacEndianness mFrom = TacEndianness::Unknown;
  TacEndianness mTo = TacEndianness::Unknown;
  TacVector< char > mBytes;
};
template<> inline void TacWriter::Write( const v2& t ){ return Write( t.data(), 2, sizeof( float ) ); }
template<> inline void TacWriter::Write( const v3& t ){ return Write( t.data(), 3, sizeof( float ) ); }
template<> inline void TacWriter::Write( const v4& t ){ return Write( t.data(), 4, sizeof( float ) ); }

