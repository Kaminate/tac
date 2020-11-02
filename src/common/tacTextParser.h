// This file defines the Tac::TextParser class, which can be used to
// extract text from a byte buffer

#pragma once
#include "src/common/tacPreprocessor.h"

namespace Tac
{
  template < typename T >
  struct Optional
  {
    Optional() : mT( ( T )0 ), mExist( false ) {}
    Optional( T t ) : mT( t ), mExist( true ) {}
    T    GetValue() { TAC_ASSERT( mExist ); return mT; }
    T    GetValueOr( T t ) { return mExist ? mT : t; }
    T    GetValueUnchecked() { return mT; }
    bool HasValue() { return mExist; }
  private:
    T    mT;
    bool mExist;
  };

  struct StringView;

  struct ParseData
  {
    ParseData( const char*, int );

    // Eat functions
    const char*       EatByte();
    const char*       EatBytes( int );
    StringView        EatRestOfLine();
    bool              EatNewLine();
    bool              EatWhitespace();
    bool              EatUntilCharIsNext( char );
    bool              EatUntilCharIsPrev( char );

    // Does not eat prepended whitespace
    bool              EatStringExpected( const StringView& );
    Optional< float > EatFloat();
    StringView        EatWord();

    // Peek functions return 0/false/nullptr if there is no space remaining
    const char*       PeekByte() const;
    const char*       PeekBytes( int ) const;
    char              PeekByteUnchecked() const;
    int               PeekWhitespace() const;
    int               PeekNewline() const;
    bool              PeekStringExpected( const StringView& ) const;
    const char*       GetPos() const;
    int               GetRemainingByteCount() const;
    const char*       mBytes;
    int               mByteCount;
    int               mIByte;
  };
}

