// This file defines the Tac::TextParser class, which can be used to
// extract text from a byte buffer

#pragma once
#include "src/common/tacPreprocessor.h"
#include "src/common/string/tacString.h"

namespace Tac
{
  // move to optional.h?
  template < typename T >
  struct Optional
  {
    Optional()      : mT( ( T )0 ), mExist( false ) {}
    Optional( T t ) : mT( t ), mExist( true ) {}
    T    GetValue() const          { TAC_ASSERT( mExist ); return mT; }
    T    GetValueOr( T t ) const   { return mExist ? mT : t; }
    T    GetValueUnchecked() const { return mT; }
    bool HasValue() const          { return mExist; }
    operator T() const             { return mT; };
  private:
    T    mT;
    bool mExist;
  };

  struct ParseData
  {
    ParseData( StringView );
    ParseData( const char* );
    ParseData( const char*, int );
    ParseData( const char*, const char* );

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

    //                Store in a StringView for better visualization of non-null-terminated strings
    //                in the debugger
    StringView        mStr;
    int               mIByte = 0;
  };
}

