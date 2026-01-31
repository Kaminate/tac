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

  template< typename T >
  struct Bitfield
  {
    Bitfield() = default;
    Bitfield( T t ) : mBitfield{ t }        {}
    bool IsSet( int i ) const               { return mBitfield & ( 1 << i ); }
    void Set( int i )                       { mBitfield |= ( 1 << i ); }
    bool Empty() const                      { return !mBitfield; }
    void SetAll()                           { mBitfield = ( T )( -1 ); }
    static auto AllSet()                    { Bitfield result; result.SetAll(); return result; }

    T mBitfield {};
  };

  // a bitfield which marks which NetVars of a component need to be networked
  struct NetVarDiff : public Bitfield< u8 >{};
  struct NetMembers : public Bitfield< u8 >{};

  struct NetVarRegistration
  {
    struct DiffParams{ const void* mOld; const void* mNew; };

    void Add( const char* );

    void Read( ReadStream*, dynmc void*, Errors& ) const;
    void Write( WriteStream*, const void*, NetVarDiff ) const;
    auto Diff( DiffParams ) const -> NetVarDiff;

    NetMembers               mNetMembers {};
    const MetaCompositeType* mMetaType   {};
  };

  struct ReadStream
  {
    template< typename T > T Read( Errors& errors ) { T t{}; ReadBytes( &t, sizeof( t ), errors ); return t; }
    void ReadBytes( void*, int, Errors& );

    Span< const char > mBytes {};
    int                mIndex {};


  private:
    const void* Advance( int );
    int Remaining() const;

  };

  struct WriteStream
  {
    template< typename T >
    void Write( Span< const T > ts )                                                                { WriteBytes( ts.data(), ts.size() * sizeof( T ) ); }

    template< typename T >
    void Write( const T& t )                                                                       { WriteBytes( &t, sizeof( T ) ); }

    //template< typename T >
    //void  Write( const T* ts, int n )                                                             { WriteBytes( &ts, sizeof( T ) ); }

    void WriteBytes( const void*, int );
    auto Data() -> void*                                                                           { return mBytes.data(); } 
    int  Size() const                                                                              { return mBytes.size(); }

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

    [[nodiscard]] auto Convert( u8 ) -> u8;
    [[nodiscard]] auto Convert( u16 ) -> u16;
    [[nodiscard]] auto Convert( u32 ) -> u32;
    [[nodiscard]] auto Convert( u64 ) -> u64;
    [[nodiscard]] auto Convert( float ) -> float;

  private:
    void Convert( void*, int );
    bool mFlip;
  };



}
