#pragma once

#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{


  struct ShortFixedString
  {
    ShortFixedString() = default;
    ShortFixedString( const char* );
    ShortFixedString( const char*, int );
    ShortFixedString( StringView );

    using Arg = StringView;

#if 0
    // I would like to force Args to be of type StringView...
    template< typename ... Args>
    static ShortFixedString Concat( Args ... args )
    {
      ShortFixedString result;
      ( result += ... += args ); // c++17 binary left fold expression
      return result;
    }
#endif

    static ShortFixedString Concat( Arg, Arg, Arg, Arg, Arg, Arg, Arg, Arg );
    static ShortFixedString Concat( Arg, Arg, Arg, Arg, Arg, Arg, Arg );
    static ShortFixedString Concat( Arg, Arg, Arg, Arg, Arg, Arg );
    static ShortFixedString Concat( Arg, Arg, Arg, Arg, Arg );
    static ShortFixedString Concat( Arg, Arg, Arg, Arg );
    static ShortFixedString Concat( Arg, Arg, Arg );
    static ShortFixedString Concat( Arg, Arg );

    //operator const char* ( ) const;

    bool        empty() const;
    int         size() const;
    int         capacity() const;
    const char* data() const;

    //void assign( const char* s )        { assign( StringView( s ) ); }
    //void assign( const char* s, int n ) { FixedStringAssign( GetFSD(), StringView( s, n ); }
    void        assign( const StringView& );
    dynmc char& front() dynmc;
    const char& front() const;
    dynmc char& back() dynmc;
    const char& back() const;
    dynmc char* begin() dynmc;
    const char* begin() const;
    dynmc char* end() dynmc;
    const char* end() const;
    dynmc char& operator []( int ) dynmc;
    const char& operator []( int ) const;
    ShortFixedString& operator += ( char );
    ShortFixedString& operator += ( const StringView& );

    operator StringView() const;


    // needed to convert the FixedString result from va()
    // into the const char* in TAC_ASSERT HandleAssert
    //operator const char*() const;

    // why does this struct exist?
    // its for `va()` inside tac_string_format.h
    struct Data
    {
      char* mBuf      {};
      int*  mSize     {};
      int   mCapacity {};
    };

    Data GetData();

  private:

    static const int N { 100 };
    int mSize          {};
    char mBuf[ N ]     { "" };
  };

  // -----------------------------------------------------------------------------------------------

} // namespace Tac

