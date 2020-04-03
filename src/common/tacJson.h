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

struct TacIndentation
{
  int spacesPerTab = 2;
  int tabCount = 0;
  bool convertTabsToSpaces = true;
  TacString ToString();
};

// The function names Stringify and Parse mimic the built-in javascript api
struct TacJson
{

  TacJson();
  TacJson( const char* str );
  TacJson( const TacString& s );
  TacJson( TacJsonNumber number );
  TacJson( int number );
  TacJson( bool b );
  TacJson( const TacJson& other );
  ~TacJson();
  void Clear();
  TacString Stringify( TacIndentation* indentation ) const;
  TacString Stringify() const;
  void Parse( const char* bytes, int byteCount, TacErrors& errors );
  void Parse( const TacString& s, TacErrors& errors );

  TacJson& GetChild( TacStringView key );
  TacJson& operator[]( const TacString& key );
  TacJson& operator[]( const char* key );
  void operator = ( const TacJson& json );
  void operator = ( const TacJson* json );
  void operator = ( const char* str );
  void operator = ( const TacString& str );
  void operator = ( TacJsonNumber number );
  void operator = ( int number );
  void operator = ( bool b );
  operator TacString ();
  operator TacJsonNumber ();
  operator bool ();

  std::map< TacString, TacJson* > mChildren;
  TacString mString;
  TacJsonNumber mNumber = 0;
  TacVector< TacJson* > mElements;
  bool mBoolean = false;
  TacJsonType mType = TacJsonType::Object;
};

