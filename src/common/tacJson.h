// This file implements the JavaScript Object Notation,
// which is a file format for de/serializing data

#pragma once

#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"

#include <map>

typedef double TacJsonNumber;

const bool tacJsonParseDebug = false;

enum class TacJsonType
{
  String,
  Number,
  Object,
  Array,
  Bool,
  Null
};

// The function names Stringify and Parse mimic the built-in javascript api
struct TacJson
{
  struct TacParseData
  {
    const char* mBytes;
    int mByteCount;
    int mIByte;
  };
  struct TacIndentation
  {
    int spacesPerTab = 2;
    int tabCount = 0;
    bool convertTabsToSpaces = true;
    TacString ToString();
  };
  TacJson();

  //template< typename T > TacJson( const T& t ) { *this = t; }

  // Used by some other client apis. unfortunately cant really combine this with the = operator
  TacJson( const char* str ) { mType = TacJsonType::String; mString = str; }
  TacJson( const TacString& s ) { mType = TacJsonType::String; mString = s; }
  TacJson( TacJsonNumber number ) { mType = TacJsonType::Number; mNumber = number; }
  TacJson( int number ) { mType = TacJsonType::Number; mNumber = ( TacJsonNumber )number; }
  TacJson( bool b ) { mType = TacJsonType::Bool; mBoolean = b; }
  TacJson( const TacJson& other );

  ~TacJson();
  void Clear();
  TacString Stringify( TacIndentation* indentation ) const;
  TacString Stringify() const;
  TacString CharToString( char c ) const;
  TacString Surround( const TacString& inner, const TacString& outer ) const;
  TacString DoubleQuote( const TacString& s ) const;
  void ByteEat( TacParseData* parseData, char& c, TacErrors& errors );
  void EatRestOfLine( TacParseData* parseData, TacErrors& errors );
  void BytePeek( TacParseData* parseData, char& c, TacErrors& errors );
  void BytePeekUnchecked( TacParseData* parseData, char& c );
  void ByteIncrement( TacParseData* parseData, int byteCount = 1 );
  void UnexpectedCharacter( char c, TacErrors& errors );
  void ExpectCharacter( char c, char expected, TacErrors& errors );
  void SkipLeadingWhitespace( TacParseData* parseData );
  void ParseNumber( TacParseData* parseData, TacJsonNumber& jsonNumber, TacErrors& errors );
  void ParseString( TacParseData* parseData, TacString& stringToParse, TacErrors& errors );
  void ParseStringExpected( TacParseData* parseData, const TacString& expected, TacErrors& errors );
  void ParseObject( TacParseData* parseData, TacErrors& errors );
  void ParseArray( TacParseData* parseData, TacErrors& errors );
  void ParseUnknownType(
    TacParseData* parseData,
    TacErrors& errors );
  void Parse( const char* bytes, int byteCount, TacErrors& errors );
  void Parse( const TacString& s, TacErrors& errors );

  // TODO: replace with string view
  TacJson& operator[]( const TacString& key );

  void operator = ( const TacJson& json );
  void operator = ( const TacJson* json );

  // Client api, ie: settings[ "foo" ][ "bar" ] = qux;
  void operator = ( const char* str ){ mType = TacJsonType::String; mString = str; }
  void operator = ( const TacString& s ){  mType = TacJsonType::String; mString = s;}
  void operator = ( TacJsonNumber number ){  mType = TacJsonType::Number; mNumber = number;}
  void operator = ( int number ){  mType = TacJsonType::Number; mNumber = ( int )number;}
  void operator = ( bool b ) {  mType = TacJsonType::Bool; mBoolean = b;}

  std::map< TacString, TacJson* > mChildren;
  TacString mString;
  TacJsonNumber mNumber = 0;
  TacVector< TacJson* > mElements;
  bool mBoolean = false;
  TacJsonType mType = TacJsonType::Object;
};

