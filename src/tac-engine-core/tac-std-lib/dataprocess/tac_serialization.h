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
//#include "tac-std-lib/meta/tac_meta_var.h"

namespace Tac
{

  //struct Reader;
  //struct Writer;

  struct WriteStream;
  struct ReadStream;

  enum class Endianness
  {
    Unknown = 0,
    Little, // Common on most intel cpu architectures
    Big, // Also called host byte order
  };

  Endianness GetEndianness();

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




  struct ReadStream
  {
    template< typename T > T Read( Errors& errors ) { T t{}; ReadBytes( &t, sizeof( t ), errors ); return t; }
    void ReadBytes( void*, int, Errors& );

    Span< const char > mBytes {};
    int                mIndex {};

    template< typename T > T ReadT( Errors& errors )
    {
      T t{};
      TAC_RAISE_ERROR_IF_RETURN( {}, ! ReadT( &t ), "ReadStream::ReadT() failed" );
      return t;
    }

  private:
    const void* Advance( int );
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
    template< typename T >
    void Write( Span< const T > ts )                                                                { WriteBytes( ts.data(), ts.size() * sizeof( T ) ); }

    template< typename T >
    void  Write( const T& t )                                                                       { WriteBytes( &t, sizeof( T ) ); }

    //template< typename T >
    //void  Write( const T* ts, int n )                                                             { WriteBytes( &ts, sizeof( T ) ); }

    void  WriteBytes( const void*, int );
    void* Data()                                                                                    { return mBytes.data(); }
    int   Size() const                                                                              { return mBytes.size(); }

  private:

    void* Advance( int );
    Vector< char > mBytes;
  };


  struct NetEndianConverter
  {
    struct Params
    {
      Endianness mFrom {};
      Endianness mTo   {};
    };

    NetEndianConverter( Params );

    [[nodiscard]] u8    Convert( u8 );
    [[nodiscard]] u16   Convert( u16 );
    [[nodiscard]] u32   Convert( u32 );
    [[nodiscard]] u64   Convert( u64 );
    [[nodiscard]] float Convert( float );


  private:
    void Convert( void*, int );
    bool mFlip;
  };



}
