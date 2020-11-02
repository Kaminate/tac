// This file implements the JavaScript Object Notation,
// which is a file format for de/serializing data

#pragma once

#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"

#include <map>

namespace Tac
{
  struct Errors;
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
    int    spacesPerTab = 2;
    int    tabCount = 0;
    bool   convertTabsToSpaces = true;
    String ToString();
  };

  // The function names Stringify and Parse mimic the built-in javascript api
  struct Json
  {
    Json();
    Json( StringView );
    Json( JsonNumber );
    Json( int );
    Json( bool );
    Json( const Json& );
    ~Json();
    void                      Clear();
    String                    Stringify( Indentation* ) const;
    String                    Stringify() const;
    void                      Parse( const char* bytes, int byteCount, Errors& );
    void                      Parse( StringView, Errors& );
    Json&                     GetChild( StringView );
    Json&                     operator[]( StringView );
    Json&                     operator[]( const char* );
    void                      operator = ( const Json& );
    void                      operator = ( const Json* );
    void                      operator = ( StringView );
    void                      operator = ( JsonNumber );
    void                      operator = ( int );
    void                      operator = ( bool );
    operator                  String ();
    operator                  JsonNumber ();
    operator                  bool();
    std::map< String, Json* > mChildren;
    String                    mString;
    JsonNumber                mNumber = 0;
    Vector< Json* >           mElements;
    bool                      mBoolean = false;
    JsonType                  mType = JsonType::Object;
  };

}
