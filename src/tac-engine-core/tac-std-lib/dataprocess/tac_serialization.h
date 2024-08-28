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
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/meta/tac_meta_var.h"

namespace Tac
{

  //struct Reader;
  //struct Writer;

  struct WriteStream;
  struct ReadStream;

  enum class Endianness
  {
    Unknown,
    Little, // Common on most intel cpu architectures
    Big, // Also called host byte order
  };

  Endianness GetEndianness();

  //struct NetVarReader     { virtual bool Read( Reader*, void* dst ) = 0; };
  //struct NetVarWriter     { virtual void Write( Writer*, const void* src ) = 0; };


#if 0

  // A NetVar describes a single member variable of a component, which is
  // networked through snapshots.
  struct NetVar
  {
    bool Equals( const void*, const void* ) const;
    //void CopyFrom( void* dst, const void* src ) const;

    //const MetaMember* mMetaMember {};
    //const char*       mDebugName           {};
    //int               mByteOffset          {};
    //int               mElementByteCount    {};
    //int               mElementCount        {};
    //NetVarReader*     mVarReader           {};
    //NetVarWriter*     mVarWriter           {};
    //bool              mIsTriviallyCopyable {};

    NetVarReader*     mVarReaders[8]           {};
    NetVarWriter*     mVarWriters[8]           {};
  };
#endif


  // a bitfield which marks which NetVars of a component need to be networked
  struct NetBitDiff
  {
    bool IsSet( int i ) const               { return mBitfield & ( 1 << i ); }
    void Set( int i )                       { mBitfield |= ( 1 << i ); }
    bool Empty() const                      { return !mBitfield; }

    u8 mBitfield {};
  };

  struct NetVarReaderWriter
  {
    void Add( const char* );
    void Read( ReadStream*, dynmc void* );
    void Write( WriteStream*, const void* );
    //NetVarReader*            mReaders[ 8 ];
    //NetVarWriter*            mWriters[ 8 ];
    NetBitDiff               mEnabled  {};
    const MetaCompositeType* mMetaType {};
  };


#if 0

  // A `NetVars` is a collection of `NetVar`, which lists all the data members of a
  // component to be networked through snapshots.
  // There is one NetVars per Component type
  //
  // couold be a typedef?
  struct NetVars
  {
    int                        size() const;
    void                       Add( NetVar );
    void                       Clear();

    // does src/dst point to a component?
    //void                       CopyFrom( void* dst, const void* src ) const;
    NetBitDiff                 Diff( const void*, const void* ) const;
    const NetVar&              operator[]( int ) const;

  private:
    FixedVector< NetVar, 10 >   mNetVars;
  };
#endif

  struct ReadStream
  {
    bool Read( char* u )                                   { return ReadT( u ); }
    bool Read( u8* u )                                     { return ReadT( u ); }
    bool Read( u16* );
    bool Read( u32* );
    bool Read( u64* );
    bool Read( float* );

    Span< const void* > mBytes {};
    int                 mIndex {};

    template< typename T > T ReadT( Errors& errors )
    {
      T t{};
      TAC_RAISE_ERROR_IF_RETURN( {}, ! ReadT( &t ), "ReadStream::ReadT() failed" );
      return t;
    }

  private:
    void* Advance( int );
    int Remaining() const;

    template< typename T > bool ReadT( T* t )
    {
      const int remaining{ Remaining() };
      if( remaining < sizeof( T ) )
        return false;

      *t = *( T* )Advance( sizeof( T ) );
      return true;
    }
  };


  struct WriteStream
  {
    void Write( u8 );
    void Write( u16 );
    void Write( u32 );
    void Write( u64 );
    void Write( float );
    void* Advance( int );
    int Size() const;

    Vector< char > mBytes;

  private:
    template< typename T > void WriteT( T t )
    {
       *( T* )Advance( sizeof( T ) ) = t;
    }
  };


  struct EndianReader
  {
    bool Read( u8* );
    bool Read( u16* );
    bool Read( u32* );
    bool Read( u64* );
    bool Read( float* );
    Endianness  mFrom   { Endianness::Unknown };
    Endianness  mTo     { Endianness::Unknown };
    ReadStream* mStream {};

  private:
    void PostRead( void*, int );
    template< typename T > bool ReadT( T* t )
    {
       if( !mStream->Read( t ) )
         return false;

       PostRead( t, sizeof( T ) );
       return true;
    }
  };

  struct EndianWriter
  {
    void Write( u8 );
    void Write( u16 );
    void Write( u32 );
    void Write( u64 );
    void Write( float );

    Endianness   mFrom   { Endianness::Unknown };
    Endianness   mTo     { Endianness::Unknown };
    WriteStream* mStream {};

  private:
    void PreWrite( void*, int );
    
    template< typename T > void WriteT( T t )
    {
       PreWrite( &t, sizeof( T ) );
       mStream->Write( t ); 
    }
  };

#if 0
  // I should separate the byte stream from the endianness stuff
  struct Reader
  {
    // return true on success, false otherwise
    bool Read( void* values, int valueCount, int sizeOfValue );
    const void* ReadBytes( int byteCount );
    bool Read( void* bytes, const NetVars& networkBits );
    void Read( void* bytes, const NetVars& networkBits, Errors& errors )
    {
      TAC_RAISE_ERROR_IF( !Read( bytes, networkBits ), "read failed" );
    }
    template< typename T > bool Read( T* values, int valueCount = 1 ) { return Read( values, valueCount, sizeof( T ) ); }
    template< typename T > T Read( Errors& errors )
    {
      T t{};
      const bool didReadSucceed{ Read( &t, 1, sizeof( T ) ) };
      TAC_RAISE_ERROR_IF_RETURN( {}, !didReadSucceed, "read failed" );
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
#endif

}
