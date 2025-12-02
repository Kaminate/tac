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
    auto EatByte() -> const char*;
    auto EatBytes( int ) -> const char*;

    // returns a string up until the next newline, then eats the newline
    auto EatRestOfLine() -> StringView;
    bool EatNewLine();
    bool EatWhitespace();
    bool EatUntilCharIsNext( char );
    bool EatUntilCharIsPrev( char );

    // Does not eat prepended whitespace
    bool EatStringExpected( StringView );
    auto EatFloat( Errors& ) -> float;
    auto EatFloat() -> Optional< float >;
    auto EatWord() -> StringView;

    // Peek functions return 0/false/nullptr if there is no space remaining
    auto PeekByte() const -> const char*;
    auto PeekBytes( int ) const -> const char*;
    auto PeekByteUnchecked() const -> char;
    auto PeekWhitespace() const -> int;
    auto PeekNewline() const -> int;
    auto PeekStringExpected( StringView ) const -> bool;
    auto GetPos() const -> const char*;
    auto GetRemainingByteCount() const -> int;

    operator bool() const;

  private:

    //                Store in a StringView for better visualization of non-null-terminated strings
    //                in the debugger
    StringView        mStr   {};
    int               mIByte {};
  };
}

