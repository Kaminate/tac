#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h" // assert
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
#if 0

  const NetVar& NetVars::operator[]( int i ) const { return mNetVars[ i ]; }
  int           NetVars::size() const              { return mNetVars.size(); }
  void          NetVars::Add( NetVar b )           { mNetVars.push_back( b ); }
  void          NetVars::Clear()                   { mNetVars.clear(); }
  NetBitDiff    NetVars::Diff( const void* oldData, const void* newData ) const
  {
    if( !oldData || !newData )
      return NetBitDiff{ 0xff };

    NetBitDiff diff{};

    const int nVars{ mNetVars.size() };
    for( int iVar{}; iVar < nVars; ++iVar )
    {
      const NetVar& var{ mNetVars[ iVar ] };
      const MetaMember* metaMember{ var.mMetaMember };

      const void* oldBits{ ( const char* )oldData + metaMember->mOffset };
      const void* newBits{ ( const char* )newData + metaMember->mOffset };

      if( !var.Equals( oldBits, newBits ) )
        diff.Set( iVar );
    }

    return diff;
  }
#endif

#if 0
  void          NetVars::CopyFrom( dynmc void* dstComponent,
                                   const void* srcComponent ) const
  {
    const int nVars{ mNetVars.size() };

    for( int iVar{}; iVar < nVars; ++iVar )
    {
      const NetVar& var{ mNetVars[ iVar ] };
      const MetaMember* metaMember{ var.mMetaMember };
      dynmc void* dst{ ( dynmc char* )dstComponent + metaMember->mOffset };
      const void* src{ ( const char* )srcComponent + metaMember->mOffset };

      var.CopyFrom( dst, src );
    }
  }
#endif

  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------

#if 0

  bool          NetVar::Equals( const void* a, const void* b ) const
  {
    const MetaType* metaType{ mMetaMember->mMetaType };
    return metaType->Equals( a, b );
    //if( mVarCompare )
    //  return mVarCompare->Equals( a, b );
    //TAC_ASSERT( mIsTriviallyCopyable );
    //const int n{ mElementCount * mElementByteCount };
    //return MemCmp( a, b, n ) == 0;
  }
#endif


#if 0
  void          NetVar::CopyFrom( void* dst, const void* src ) const
  {
    const MetaType* metaType{ mMetaMember->mMetaType };
    metaType->Copy( dst, src );

    if( mVarWriter || mVarReader )
    {
      TAC_ASSERT( mVarWriter && mVarReader );

      const Endianness endianness{ GetEndianness() };
      Writer writer
      {
        .mFrom { endianness },
        .mTo   { endianness },
      };

      mVarWriter->Write( &writer, src );

      Reader reader
      {
        .mFrom  { endianness },
        .mTo    { endianness },
        .mBegin { writer.mBytes.data() },
        .mEnd   { writer.mBytes.data() + writer.mBytes.size() },
      };
      const bool readSuccess{ mVarReader->Read( &reader, dst ) };
      TAC_ASSERT( readSuccess );
    }
    else
    {
      const int size { mElementByteCount * mElementCount };
      TAC_ASSERT( size );
      TAC_ASSERT( mIsTriviallyCopyable );
      MemCpy( dst, src, size );
    }
  }
#endif

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

  static void Copy( void* dst, const void* src, int size, Endianness a, Endianness b )
  {
    CopyValueAccountForEndianness( dst, src, size, a, b );
  }

  // -----------------------------------------------------------------------------------------------
#if 0

  bool Reader::Read( void* values, int valueCount, int sizeOfValue )
  {
    TAC_ASSERT( mFrom != Endianness::Unknown );
    TAC_ASSERT( mTo != Endianness::Unknown );
    auto dstDiff { ( char* )mEnd - ( char* )mBegin };
    auto srcDiff { ( std::intptr_t )valueCount * ( intptr_t )sizeOfValue };
    if( dstDiff < srcDiff )
      return false;
    for( int i{}; i < valueCount; ++i )
      CopyValueAccountForEndianness( ( char* )values + ( intptr_t )sizeOfValue * i,
      ( char* )mBegin + ( intptr_t )sizeOfValue * i,
                                     sizeOfValue, mFrom, mTo );
    mBegin = ( char* )mBegin + ( intptr_t )sizeOfValue * valueCount;
    return true;
  }

  const void* Reader::ReadBytes( int byteCount )
  {
    const void* newBegin{ ( char* )mBegin + byteCount };
    if( newBegin > mEnd )
      return nullptr;

    const void* result{ mBegin };
    mBegin = newBegin;
    return result;
  }

  bool Reader::Read( void* bytes, const NetVars& vars )
  {
    NetBitDiff netBitDiff;
    if( !Read( &netBitDiff.mBitfield ) )
      return false;

    TAC_ASSERT( !netBitDiff.Empty() ); // y u sending me nothin
    const int nVars{ vars.size() };
    for( int iVar {}; iVar < nVars; ++iVar )
    {
      if( !netBitDiff.IsSet( iVar ) )
        continue;

      const NetVar& var { vars[ iVar ] };
      const MetaMember* metaMember{ var.mMetaMember };
      const MetaType* metaType{ metaMember->mMetaType };

      void* dst{ ( char* )bytes + metaMember->mOffset };

      if( var.mIsTriviallyCopyable )
      {
        const void* src{ ReadBytes( metaType->GetSizeOf() ) };
        if( !src )
          return false;

        metaType->Copy( { .mDst{ dst }, .mSrc{ src }, } );
      }
      else
      {
        if( !var.mVarReader->Read( this, dst ) )
        {
          return false;
        }
      }
    }

    return true;
  }

  // -----------------------------------------------------------------------------------------------

  void Writer::Write( const void* values,
                      int valueCount,
                      int sizeOfValue )
  {
    TAC_ASSERT( mFrom != Endianness::Unknown );
    TAC_ASSERT( mTo != Endianness::Unknown );
    const int oldSize { mBytes.size() };
    mBytes.resize( oldSize + sizeOfValue * valueCount );
    for( int i{}; i < valueCount; ++i )
    {
      void* dst { ( char* )mBytes.data() + std::ptrdiff_t( oldSize + i * sizeOfValue) };
      const void* src { ( const char* )values + sizeOfValue * i };
      CopyValueAccountForEndianness( dst, src, sizeOfValue, mFrom, mTo );
    }
  }
  void Writer::Write( const void* bytes,
                      NetBitDiff diff,
                      const NetVars& vars )
  {
    const u8 bitfield { diff.mBitfield };
    TAC_ASSERT( bitfield );

    Write( bitfield );
    const int nVars{ vars.size() };
    for( int iVar {}; iVar < nVars; ++iVar )
    {
      if( !diff.IsSet( iVar ) )
        continue;

      const NetVar& var { vars[ iVar ] };
      const MetaMember* metaMember{ var.mMetaMember };
      //const MetaType* metaType{ metaMember->mMetaType };
      const void* varBytes{ ( char* )bytes + metaMember->mOffset};
      var.mVarWriter->Write( this, varBytes );
    }
  }
#else

  // -----------------------------------------------------------------------------------------------

  void NetVarReaderWriter::Add( const char* name )
  {
    TAC_ASSERT( mMetaType );
    const int n{ mMetaType->GetMemberCount() };
    TAC_ASSERT( n <= 8 );
    for( int i{}; i < n; ++i )
    {
      const MetaMember& metaMember{ mMetaType->GetMember( i ) };
      if( metaMember.mName == ( StringView )name )
      {
        mEnabled.Set( i );
        return;
      }
    }

    TAC_ASSERT_INVALID_CODE_PATH;
  }

  void NetVarReaderWriter::Read( ReadStream* stream, dynmc void* basePtr )
  {
    TAC_ASSERT( mMetaType );
    TAC_ASSERT( stream );
    TAC_ASSERT( basePtr );
    const int n{ mMetaType->GetMemberCount() };
    for( int i{}; i < n; ++i )
    {
      if( mEnabled.IsSet( i ) )
      {
        const MetaMember& metaMember{ mMetaType->GetMember( i ) };
        metaMember.mMetaType->Read( stream, ( char* )basePtr + metaMember.mOffset );
      }
    }
  }

  void NetVarReaderWriter::Write( WriteStream* stream, const void* basePtr )
  {
    TAC_ASSERT( mMetaType );
    TAC_ASSERT( stream );
    TAC_ASSERT( basePtr );
    const int n{ mMetaType->GetMemberCount() };
    for( int i{}; i < n; ++i )
    {
      if( mEnabled.IsSet( i ) )
      {
        const MetaMember& metaMember{ mMetaType->GetMember( i ) };
        metaMember.mMetaType->Write( stream, ( char* )basePtr + metaMember.mOffset );
      }
    }
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

#if 0
  const void* ReadStream::Advance( int n )
  {
    if( mIndex + n > mBytes.size() )
      return nullptr;

    const void* result { &mBytes[ mIndex ] };
    mIndex += n;
    return result;
  }
  bool ReadStream::Read( char* v )   { return ReadT( v ); }
  bool ReadStream::Read( u8* v )     { return ReadT( v ); }
  bool ReadStream::Read( u16* v )    { return ReadT( v ); }
  bool ReadStream::Read( u32* v )    { return ReadT( v ); }
  bool ReadStream::Read( u64* v )    { return ReadT( v ); }
  bool ReadStream::Read( float* v )  { return ReadT( v ); }
  int ReadStream::Remaining() const { return mBytes.size() - mIndex; }
#endif

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

#if 0
  bool EndianReader::Read( u8* v )    { return ReadT( v ); }
  bool EndianReader::Read( u16* v )   { return ReadT( v ); }
  bool EndianReader::Read( u32* v )   { return ReadT( v ); }
  bool EndianReader::Read( u64* v )   { return ReadT( v ); }
  bool EndianReader::Read( float* v ) { return ReadT( v ); }
  void EndianReader::PostRead( void* v, int n ) { CheckFlip( v, n, mFrom, mTo ); }

  // -----------------------------------------------------------------------------------------------
  void EndianWriter::Write( u8 v )    { WriteT( v ); }
  void EndianWriter::Write( u16 v )   { WriteT( v ); }
  void EndianWriter::Write( u32 v )   { WriteT( v ); }
  void EndianWriter::Write( u64 v )   { WriteT( v ); }
  void EndianWriter::Write( float v ) { WriteT( v ); }
  void EndianWriter::PreWrite( void* v, int n) { CheckFlip( v, n, mFrom, mTo ); }
#endif

#endif
} // namespace Tac

