// This file is used to shove data in and out of buffers
// ( which may be files or network streams )

#pragma once

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/tac_ints.h"

namespace Tac
{

  struct Reader;
  struct Writer;

  enum class Endianness
  {
    Unknown,
    Little, // Common on most intel cpu architectures
    Big, // Also called host byte order
  };

  Endianness GetEndianness();

  struct NetVarReader
  {
    virtual bool Read( Reader*, void* dst ) = 0;
  };

  struct NetVarWriter
  {
    virtual void Write( Writer*, const void* src ) = 0;
  };

  // A NetVar (terrible name) describes a single member variable of a component, which is
  // networked through snapshots.
  struct NetVar
  {
    const char*       mDebugName        {};
    int               mByteOffset       {};
    int               mElementByteCount {};
    int               mElementCount     {};
    NetVarReader*     mVarReader        {};
    NetVarWriter*     mVarWriter        {};
  };

  // A `NetVars` is a collection of `NetVar`, which lists all the data members of a
  // component to be networked through snapshots.
  // There is one NetVars per Component type
  //
  // couold be a typedef?
  struct NetVars
  {
    const NetVar&               operator[]( int ) const;
    int                             size() const;
    void                            Add( NetVar );
    void                            Clear() { mNetVars.clear(); }
  private:
    FixedVector< NetVar, 10 >   mNetVars;
  };

  // a bitfield which marks which NetVars of a component need to be networked
  struct NetBitDiff
  {
    bool IsSet( int i ) const { return mBitfield & ( 1 << i ); }
    bool Empty() const        { return !mBitfield; }

    u8 mBitfield {};
  };


  // I should separate the byte stream from the endianness stuff
  struct Reader
  {
    bool Read( void* values, int valueCount, int sizeOfValue );
    bool Read( void* bytes, const NetVars& networkBits );
    void Read( void* bytes, const NetVars& networkBits, Errors& errors )
    {
      TAC_RAISE_ERROR_IF( !Read( bytes, networkBits ), "read failed" );
    }
    template< typename T > bool Read( T* values, int valueCount = 1 ) { return Read( values, valueCount, sizeof( T ) ); }
    template< typename T > T Read( Errors& errors )
    {
      T t;
      TAC_RAISE_ERROR_IF_RETURN( !Read( &t, 1, sizeof( T ) ), "read failed", t );
      return t;
    }
    Endianness  mFrom  { Endianness::Unknown };
    Endianness  mTo    { Endianness::Unknown };
    const void* mBegin {};
    const void* mEnd   {};
  };

  template<> inline bool Reader::Read( v2* values, int valueCount ) { return Read( values->data(), 2 * valueCount, sizeof( float ) ); }
  template<> inline bool Reader::Read( v3* values, int valueCount ) { return Read( values->data(), 3 * valueCount, sizeof( float ) ); }
  template<> inline bool Reader::Read( v4* values, int valueCount ) { return Read( values->data(), 4 * valueCount, sizeof( float ) ); }

  // I should separate the byte stream from the endianness stuff
  struct Writer
  {
    void Write( const void* values, int valueCount, int sizeOfValue );
    void Write( const void* bytes, NetBitDiff bitfield, const NetVars& networkBits );
    template< typename T > void Write( T t )                    { return Write( &t, 1, sizeof( T ) ); }
    template< typename T > void Write( const T* t, int tCount ) { return Write( t, tCount, sizeof( T ) ); }
    Endianness     mFrom { Endianness::Unknown };
    Endianness     mTo   { Endianness::Unknown };
    Vector< char > mBytes;
  };
  template<> inline void Writer::Write( const v2& t ) { return Write( t.data(), 2, sizeof( float ) ); }
  template<> inline void Writer::Write( const v3& t ) { return Write( t.data(), 3, sizeof( float ) ); }
  template<> inline void Writer::Write( const v4& t ) { return Write( t.data(), 4, sizeof( float ) ); }

}
