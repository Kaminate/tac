#pragma once

namespace Tac
{
  struct StringView;

  struct FixedStringData
  {
    char* mBuf = nullptr;
    int*  mSize = nullptr;
    int   mCapacity = 0;
  };

  // move to static fn inside fixedstringdata?
  void FixedStringAssign( const FixedStringData&, const StringView& );
  void FixedStringAppend( const FixedStringData&, const StringView& );

  struct ShortFixedString
  {
    ShortFixedString() = default;
    ShortFixedString( const char* );
    ShortFixedString( const char* , int );
    ShortFixedString( const StringView& );

    static ShortFixedString Concat( const StringView& ,
                                    const StringView& ,
                                    const StringView& ,
                                    const StringView& ,
                                    const StringView& );

    static ShortFixedString Concat( const StringView& ,
                                    const StringView& ,
                                    const StringView& ,
                                    const StringView& );

    static ShortFixedString Concat( const StringView& ,
                                    const StringView& ,
                                    const StringView& );

    static ShortFixedString Concat( const StringView& ,
                                    const StringView& );

    //operator const char* ( ) const;

    int         size() const;
    int         capacity() const;
    const char* data() const;

    //void assign( const char* s )        { assign( StringView( s ) ); }
    //void assign( const char* s, int n ) { FixedStringAssign( GetFSD(), StringView( s, n ); }
    void        assign( const StringView& );
    char&       front();
    char        front() const;
    char&       back();
    char        back() const;
    char*       begin();
    const char* begin() const;
    char*       end();
    const char* end() const;
    char& operator []( int );
    char operator []( int ) const;
    void operator += ( char );
    void operator += ( const StringView& );

    operator StringView() const               { return StringView( mBuf, mSize ); }


    // needed to convert the FixedString result from va()
    // into the const char* in TAC_ASSERT HandleAssert
    //operator const char*() const;

    FixedStringData GetFSD();

  private:

    static const int N = 100;

    int mSize = 0;
    char mBuf[ N ] = "";
  };

  // -----------------------------------------------------------------------------------------------

}

