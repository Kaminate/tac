#include "tac_json.h" // self-inc

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  static String DoubleQuote( StringView s )
  {
    String quote = String( 1, '\"' );
    return quote + String( s ) + quote;
  }

  static void ParseObject( Json* json, ParseData* parseData, Errors& errors );

  static void ParseArray( Json* json, ParseData* parseData, Errors& errors );

  static void ParseQuotedString( StringView* stringView, ParseData* parseData, Errors& errors )
  {
    if( parseData->GetRemainingByteCount() < 2 )
      TAC_RAISE_ERROR( "not enough space for quoted string");
    const char* quotedStrBegin { parseData->GetPos() };
    if( *quotedStrBegin != '\"' )
      TAC_RAISE_ERROR( "string does not begin with quote");
    parseData->EatByte();
    if( !parseData->EatUntilCharIsPrev( '\"' ) )
      TAC_RAISE_ERROR( "string does not end with quote");
    const char* quotedStrEnd = parseData->GetPos();
    *stringView = StringView( quotedStrBegin + 1, quotedStrEnd - 1 );
  }

  static void ParseUnknownType( Json* json, ParseData* parseData, Errors& errors )
  {
    json->Clear();
    parseData->EatWhitespace();
    const char* b { parseData->PeekByte() };
    if( !b )
      return;

    const char c { *b };
    if( c == '{' )
    {
      ParseObject( json, parseData, errors );
    }
    else if( c == '[' )
    {
      ParseArray( json, parseData, errors );
    }
    else if( c == '\"' )
    {
      StringView s;
      TAC_CALL( ParseQuotedString( &s, parseData, errors ));
      json->SetString( s );
    }
    else if( ( c >= '0' && c <= '9' ) || c == '-' || c == '.' )
    {
      auto f = parseData->EatFloat();
      TAC_RAISE_ERROR_IF( !f.HasValue(), "Failed to parse number" );
      json->SetNumber( ( JsonNumber )f.GetValue() );
    }
    else if( c == 'n' )
    {
      if( !parseData->EatStringExpected( "null" ) )
        TAC_RAISE_ERROR( "Failed parsing null" );
      json->mType = JsonType::Null;
    }
    else if( c == 't' )
    {
      const StringView str = "true";
      if( !parseData->EatStringExpected( str ) )
        TAC_RAISE_ERROR( "encountered t but no true" );
      json->SetBool( true );
    }
    else if( c == 'f' )
    {
      const StringView str = "false";
      if( !parseData->EatStringExpected( str ) )
        TAC_RAISE_ERROR( "encountered f but no false" );
      json->SetBool( false );
    }
    else if( c == '/' )
    {
      parseData->EatRestOfLine();
      ParseUnknownType( json, parseData, errors );
    }
    else
    {
      const String errorMsg = String( "Unexpected character: " ) + c;
      TAC_RAISE_ERROR( errorMsg );
    }
  }

  static void ParseObject( Json* json, ParseData* parseData, Errors& errors )
  {
    json->mType = JsonType::Object;
    parseData->EatWhitespace();
    if( !parseData->EatStringExpected( "{" ) )
      TAC_RAISE_ERROR( "Expected {" );

    for( ;; )
    {
      parseData->EatWhitespace();
      if( parseData->EatStringExpected( "}" ) )
        break;

      if( parseData->EatStringExpected( "," ) )
        continue;

      if( parseData->EatStringExpected( "//" ) )
      {
        parseData->EatRestOfLine();
        continue;
      }

      StringView key;
      TAC_CALL( ParseQuotedString( &key, parseData, errors ));
      TAC_RAISE_ERROR_IF( key.empty(), "object key cannot be empty" );
      if( !parseData->EatUntilCharIsPrev( ':' ) )
        TAC_RAISE_ERROR( "Missing : after json key" );
      TAC_CALL( ParseUnknownType( &json->GetChild( key ), parseData, errors ));
    }
  }

  static void ParseArray( Json* json, ParseData* parseData, Errors& errors )
  {
    json->mType = JsonType::Array;
    parseData->EatWhitespace();
    if( !parseData->EatStringExpected( "[" ) )
      TAC_RAISE_ERROR( "Expected [");
    for( ;; )
    {
      parseData->EatWhitespace();
      if( parseData->EatStringExpected( "]" ) )
        break;

      if( parseData->EatStringExpected( "," ) )
        continue;

      // Handle comments
      if( parseData->EatStringExpected( "//" ) )
      {
        parseData->EatRestOfLine();
        continue;
      }

      TAC_CALL( ParseUnknownType( json->AddChild(), parseData, errors ));
    }
  }

  static String Tab( const Indentation* indentation, int tabCount )
  {
    const Indentation defaultIndentation;
    indentation = indentation ? indentation : &defaultIndentation;
    String tab { indentation->convertTabsToSpaces ? String( indentation->spacesPerTab, ' ' ) : String( "\t" ) };
    String result;
    for( int i{}; i < tabCount; ++i )
      result += tab;
    return result;
  }

  Json::Json( const Json& json )              { DeepCopy( &json ); }
  Json::Json( String v )                      { SetString( v ); }
  Json::Json( const char* v )                 { SetString( v ); }
  Json::Json( StringView v )                  { SetString( v ); }
  Json::Json( JsonNumber v )                  { SetNumber( v ); }
  Json::Json( bool v )                        { SetBool( v ); }
  Json::Json(){ SetNull(); }
  Json::Json( Json&& json ) noexcept          { TakeOver( move( json ) ); };
  Json::~Json()                               { Clear(); }
  Json::operator String()                     { return mString; }
  Json::operator JsonNumber()                 { return mNumber; }
  Json::operator bool()                       { return mBoolean; }
  void   Json::SetNull()                      { mType = JsonType::Null; }
  void   Json::SetNumber( JsonNumber number ) { mType = JsonType::Number; mNumber = number; }
  void   Json::SetString( StringView str )    { mType = JsonType::String; mString = str; }
  void   Json::SetBool( bool b )              { mType = JsonType::Bool; mBoolean = b; }
  void   Json::Clear()
  {
    for( auto& pair : mObjectChildrenMap )
    {
      Json* json { pair.second };
      TAC_DELETE json;
    }

    mObjectChildrenMap.clear();

    for( Json* element : mArrayElements )
      TAC_DELETE element;
    mArrayElements.clear();
  }
  String Json::Stringify( const Indentation* indentation, int tabCount ) const
  {
    int iChild {};
    String result;
    auto GetSeparator = [&]( int childCount ) { return iChild++ != childCount - 1 ? "," : ""; };
    switch( mType )
    {
      case JsonType::String:
      {
        result = DoubleQuote( mString );
      } break;
      case JsonType::Number:
      {
        if( ( ( JsonNumber )( ( int )mNumber ) ) == mNumber )
          result = ToString( ( int )mNumber );
        else
          result = ToString( mNumber );
      } break;
      case JsonType::Null:
      {
        result = "null";
      } break;
      case JsonType::Bool:
      {
        result = mBoolean ? "true" : "false";
      } break;
      case JsonType::Object:
      {
        result += Tab( indentation, tabCount ) + "{\n";
        tabCount++;
        for( auto [childKey, childValue ] : mObjectChildrenMap )
        {

          result += Tab( indentation, tabCount ) + DoubleQuote( childKey ) + ":";
          result += ( childValue->mType == JsonType::Array ||
                      childValue->mType == JsonType::Object ) ? "\n" : " ";
          result += childValue->Stringify( indentation, tabCount );
          result += GetSeparator( ( int )mObjectChildrenMap.size() );
          result += "\n";
        }
        tabCount--;
        result += Tab( indentation, tabCount ) + "}";
      } break;
      case JsonType::Array:
      {
        result += Tab( indentation, tabCount ) + "[\n";
        tabCount++;
        for( Json* element : mArrayElements )
        {
          if( !( element->mType == JsonType::Array || element->mType == JsonType::Object ) )
            result += Tab( indentation, tabCount );
          result +=
            element->Stringify( indentation, tabCount ) +
            GetSeparator( ( int )mArrayElements.size() ) +
            "\n";
        }
        tabCount--;
        result += Tab( indentation, tabCount ) + "]";
      } break;
    }
    return result;
  }

  void   Json::TakeOver( Json&& json ) noexcept
  {
    mObjectChildrenMap = json.mObjectChildrenMap;
		mString = json.mString;
		mNumber = json.mNumber;
    mArrayElements = json.mArrayElements;
		mBoolean = json.mBoolean;
    mType = json.mType;

    json.mObjectChildrenMap.clear();
    json.mArrayElements.clear();
    json.mType = JsonType::Null;
  };


  [[nodiscard]] Json   Json::Parse( const char* bytes, int byteCount, Errors& errors )
  {
    ParseData parseData( bytes, byteCount );

    Json json;
    ParseUnknownType( &json, &parseData, errors );

    return json;
  }

  [[nodiscard]] Json   Json::Parse( StringView s, Errors& errors )
  {
    ParseData parseData( s.data(), s.size() );

    Json json;
    ParseUnknownType( &json, &parseData, errors );

    return json;
  }

  Json*  Json::FindChild( StringView key ) const
  {
    auto it{ mObjectChildrenMap.find( key ) };
    return it == mObjectChildrenMap.end() ? nullptr : ( *it ).second;
  }

  Json&  Json::GetChild( StringView key )
  {
    if( Json * child{ mObjectChildrenMap[ key ] } )
      return *child;

    auto child { TAC_NEW Json };
    child->mType = JsonType::Null;

    mObjectChildrenMap[ key ] = child;
    mType = JsonType::Object;
    return *child;
  }
  Json&  Json::operator[]( StringView key )    { return GetChild( key ); }
  Json&  Json::operator[]( const char* key )   { return GetChild( key ); }
  Json&  Json::operator[]( [[maybe_unused]] int i )
  {
    mType = JsonType::Array;

    while( mArrayElements.size() <= i )
      AddChild();

    return *mArrayElements[ i ];
  }
  Json&  Json::operator = ( const Json& json )     { DeepCopy( &json ); return *this; }
  Json&  Json::operator = ( const Json* json )     { DeepCopy( json );return *this;  }
  Json&  Json::operator = ( Json&& json ) noexcept { TakeOver( move( json ) ); return *this; }
  Json&  Json::operator = ( StringView v )         { SetString( v ); return *this; }
  Json&  Json::operator = ( JsonNumber v )         { SetNumber( v ); return *this; }
  Json&  Json::operator = ( int v )                { SetNumber( ( JsonNumber )v );return *this;  }
  Json&  Json::operator = ( bool v )               { SetBool( v ); return *this; }

  void   Json::AddChild( const Json& json )
  {
    Json* child { AddChild() };
    *child = json;
  }

  Json*  Json::AddChild()
  {
    mType = JsonType::Array;
    auto child { TAC_NEW Json };
    mArrayElements.push_back( child );
    return child;
  }

  bool   Json::HasChild( StringView key )
  {
    return mObjectChildrenMap.contains( key );
  }

  Json*  Json::AddChild( StringView key )
  {
    mType = JsonType::Object;
    auto child { TAC_NEW Json };
    mObjectChildrenMap[ key ] = child;
    return child;
  }

  void   Json::DeepCopy( const Json* json )
  {
    mType = json->mType;
    mString = json->mString;
    mNumber = json->mNumber;
    mBoolean = json->mBoolean;
    mObjectChildrenMap.clear();
    mArrayElements.clear();

    for( auto [childKey, childJson] : json->mObjectChildrenMap )
      GetChild( childKey ).DeepCopy( childJson );

    for( const Json* child : json->mArrayElements )
      AddChild()->DeepCopy( child );
  }



}
