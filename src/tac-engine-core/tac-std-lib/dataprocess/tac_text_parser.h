// This file defines the Tac::TextParser class, which can be used to
// extract text from a byte buffer

#pragma once

//#include "tac-std-lib/preprocess/tac_preprocessor.h"
//#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/containers/tac_optional.h"

namespace Tac
{
  struct Errors;

  struct ParseData
  {
    ParseData( StringView );
    ParseData( const char* );
    ParseData( const char*, int );
    ParseData( const char*, const char* );

    // Eat functions
    const char*       EatByte();
    const char*       EatBytes( int );

    // returns a string up until the next newline, then eats the newline
    StringView        EatRestOfLine();
    bool              EatNewLine();
    bool              EatWhitespace();
    bool              EatUntilCharIsNext( char );
    bool              EatUntilCharIsPrev( char );

    // Does not eat prepended whitespace
    bool              EatStringExpected( const StringView& );
    float             EatFloat( Errors& );
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

    operator bool() const;

  private:

    //                Store in a StringView for better visualization of non-null-terminated strings
    //                in the debugger
    StringView        mStr;
    int               mIByte = 0;
  };
}

