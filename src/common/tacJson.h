// This file implements the JavaScript Object Notation,
// which is a file format for de/serializing data

#pragma once

#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"



#include <map>

namespace Tac
{


typedef double JsonNumber;

const bool JsonParseDebug = false;

enum class JsonType
{
  String,
  Number,
  Object,
  Array,
  Bool,
  Null
};

struct Indentation
{
  int spacesPerTab = 2;
  int tabCount = 0;
  bool convertTabsToSpaces = true;
  String ToString();
};

// The function names Stringify and Parse mimic the built-in javascript api
struct Json
{

  Json();
  Json( StringView s );
  Json( JsonNumber number );
  Json( int number );
  Json( bool b );
  Json( const Json& other );
  ~Json();
  void Clear();
  String Stringify( Indentation* indentation ) const;
  String Stringify() const;
  void Parse( const char* bytes, int byteCount, Errors& errors );
  void Parse( StringView, Errors& errors );

  Json& GetChild( StringView key );
  Json& operator[]( StringView key );
  void operator = ( const Json& json );
  void operator = ( const Json* json );
  void operator = ( StringView str );
  void operator = ( JsonNumber number );
  void operator = ( int number );
  void operator = ( bool b );
  operator String ();
  operator JsonNumber ();
  operator bool ();

  std::map< String, Json* > mChildren;
  String mString;
  JsonNumber mNumber = 0;
  Vector< Json* > mElements;
  bool mBoolean = false;
  JsonType mType = JsonType::Object;
};

}
