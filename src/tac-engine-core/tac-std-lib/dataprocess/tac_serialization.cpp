#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h" // assert
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------



  // -----------------------------------------------------------------------------------------------

}

Tac::Endianness Tac::GetEndianness()
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

namespace Tac
{




  // -----------------------------------------------------------------------------------------------

  // -----------------------------------------------------------------------------------------------

  void NetVarRegistration::Add( const char* name )
  {
    TAC_ASSERT( mMetaType );
    const int n{ mMetaType->GetMemberCount() };
    TAC_ASSERT( n <= 8 );
    for( int i{}; i < n; ++i )
    {
      const MetaMember& metaMember{ mMetaType->GetMember( i ) };
      if( metaMember.mName == ( StringView )name )
      {
        mNetMembers.Set( i );
        return;
      }
    }

    TAC_ASSERT_INVALID_CODE_PATH;
  }

  void NetVarRegistration::Read( ReadStream* stream, dynmc void* basePtr, Errors& errors ) const
  {
    TAC_ASSERT( mMetaType );
    TAC_ASSERT( stream );
    TAC_ASSERT( basePtr );

    TAC_CALL( const NetVarDiff diff{ stream->Read< NetVarDiff >( errors ) } );

    const int n{ mMetaType->GetMemberCount() };
    for( int i{}; i < n; ++i )
    {
      if( mNetMembers.IsSet( i ) && diff.IsSet(i) )
      {
        const MetaMember& metaMember{ mMetaType->GetMember( i ) };
        metaMember.mMetaType->Read( stream, ( char* )basePtr + metaMember.mOffset );
      }
    }
  }

  void NetVarRegistration::Write( WriteStream* stream, const void* basePtr, NetVarDiff diff ) const
  {
    TAC_ASSERT( mMetaType );
    TAC_ASSERT( stream );
    TAC_ASSERT( basePtr );

    stream->Write( diff.mBitfield );

    const int n{ mMetaType->GetMemberCount() };
    for( int i{}; i < n; ++i )
    {
      if( mNetMembers.IsSet( i ) && diff.IsSet( i ) )
      {
        const MetaMember& metaMember{ mMetaType->GetMember( i ) };
        metaMember.mMetaType->Write( stream, ( char* )basePtr + metaMember.mOffset );
      }
    }
  }

  NetVarDiff NetVarRegistration::Diff( DiffParams diffParams ) const
  {
    NetVarDiff result;

    const void* oldObj{ diffParams.mOld };
    const void* newObj{ diffParams.mNew };

    TAC_ASSERT( newObj );
    if( !oldObj )
    {
      result.SetAll();
      return result;
    }

    const int n{ mMetaType->GetMemberCount() };
    for( int i{}; i < n; ++i )
    {
      if( mNetMembers.IsSet( i ) )
      {
        const MetaMember& metaMember{ mMetaType->GetMember( i ) };
        const void* oldMember{ ( const char* )oldObj + metaMember.mOffset };
        const void* newMember{ ( const char* )newObj + metaMember.mOffset };
        const bool isEqual{ metaMember.mMetaType->Equals( oldMember, newMember ) };
        if( !isEqual )
          result.Set( i );
      }
    }

    return result;
  }

  // -----------------------------------------------------------------------------------------------

  void  WriteStream::WriteBytes( const void* src, int n )
  {
    dynmc void* dst{ Advance( n ) };
    MemCpy( dst, src, n );
  }

  void* WriteStream::Advance( int n )
  {
    const int oldSize{ mBytes.size() };
    const int newSize{ oldSize + n };
    mBytes.resize( newSize );
    return &mBytes[ oldSize ];
  }



  // -----------------------------------------------------------------------------------------------

  const void* ReadStream::Advance( int n )
  {
    if( mIndex + n > mBytes.size() )
      return nullptr;

    const void* result { &mBytes[ mIndex ] };
    mIndex += n;
    return result;
  }
  int         ReadStream::Remaining() const { return mBytes.size() - mIndex; }

  void        ReadStream::ReadBytes( void* dst, int n, Errors& errors )
  {
    TAC_UNUSED_PARAMETER( dst );
    TAC_UNUSED_PARAMETER( n );
    TAC_UNUSED_PARAMETER( errors );
    TAC_ASSERT_UNIMPLEMENTED;
  }

  // -----------------------------------------------------------------------------------------------

  // -----------------------------------------------------------------------------------------------

  NetEndianConverter::NetEndianConverter( Params params )
  {
    TAC_ASSERT( params.mFrom != Endianness::Unknown );
    TAC_ASSERT( params.mTo != Endianness::Unknown );
    mFlip = params.mFrom != params.mTo;
  }

  void  NetEndianConverter::Convert( void* v, int n )
  {
    if( mFlip )
      Reverse( ( char* )v, ( char* )v + n );
  }
  u8    NetEndianConverter::Convert( u8 v )    { Convert( &v, sizeof( u8 ) ); return v; }
  u16   NetEndianConverter::Convert( u16 v )   { Convert( &v, sizeof( u16 ) ); return v; }
  u32   NetEndianConverter::Convert( u32 v )   { Convert( &v, sizeof( u32 ) ); return v; }
  u64   NetEndianConverter::Convert( u64 v )   { Convert( &v, sizeof( u64 ) ); return v; }
  float NetEndianConverter::Convert( float v ) { Convert( &v, sizeof( float ) ); return v; }

  // -----------------------------------------------------------------------------------------------


} // namespace Tac

