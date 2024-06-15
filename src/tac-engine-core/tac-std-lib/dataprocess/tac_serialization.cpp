#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h" // assert
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  const NetVar& NetVars::operator[]( int i ) const { return mNetVars[ i ]; }
  int           NetVars::size() const              { return mNetVars.size(); }
  void          NetVars::Add( NetVar b )           { mNetVars.push_back( b ); }
  void          NetVars::Clear()                   { mNetVars.clear(); }
  NetBitDiff    NetVars::Diff( const void* oldData, const void* newData ) const
  {
    if( !oldData || !newData )
      return NetBitDiff{ 0xff };

    NetBitDiff diff;

    const int nVars{ mNetVars.size() };
    for( int iVar{}; iVar < nVars; ++iVar )
    {
      const NetVar& var{ mNetVars[ iVar ] };
      const void* oldBits{ ( const char* )oldData + var.mByteOffset };
      const void* newBits{ ( const char* )newData + var.mByteOffset };

      if( !var.Equals( oldBits, newBits ) )
        diff.Set( iVar );
    }

    return diff;
  }
  void          NetVars::CopyFrom( void* dstComponent, const void* srcComponent ) const
  {
    const int nVars{ mNetVars.size() };

    for( int iVar{}; iVar < nVars; ++iVar )
    {
      const NetVar& var{ mNetVars[ iVar ] };
      void* dst{ ( char* )dstComponent + var.mByteOffset };
      const void* src{ ( const char* )srcComponent + var.mByteOffset };

      var.CopyFrom( dst, src );
    }
  }

  // -----------------------------------------------------------------------------------------------

  bool          NetBitDiff::IsSet( int i ) const   { return mBitfield & ( 1 << i ); }
  void          NetBitDiff::Set( int i )           { mBitfield |= ( 1 << i ); }
  bool          NetBitDiff::Empty() const          { return !mBitfield; }

  // -----------------------------------------------------------------------------------------------

  bool          NetVar::Equals( const void* a, const void* b ) const
  {
    if( mVarCompare )
      return mVarCompare->Equals( a, b );

    TAC_ASSERT( mIsTriviallyCopyable );
    const int n{ mElementCount * mElementByteCount };
    return MemCmp( a, b, n ) == 0;
  }

  void          NetVar::CopyFrom( void* dst, const void* src ) const
  {
    if( mVarWriter && mVarReader )
    {
      Endianness endianness{ GetEndianness() };
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

  // -----------------------------------------------------------------------------------------------

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

  bool Reader::Read( void* bytes, const NetVars& vars )
  {
    u8 bitfield;
    if( !Read( &bitfield ) )
      return false;

    TAC_ASSERT( bitfield ); // y u sending me nothin
    const int nVars{ vars.size() };
    for( int iVar {}; iVar < nVars; ++iVar )
    {
      if( !( bitfield & ( 1 << iVar ) ) )
        continue;

      const NetVar& var { vars[ iVar ] };
      void* varBytes{ ( char* )bytes + var.mByteOffset };

      if( !var.mVarReader->Read( this, varBytes ) )
        return false;
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
      const void* varBytes{ ( char* )bytes + var.mByteOffset};
      var.mVarWriter->Write( this, varBytes );
    }
  }
} // namespace Tac

