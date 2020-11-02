#include "src/common/tacJson.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacMemory.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacTextParser.h"

namespace Tac
{
  static String DoubleQuote( StringView s )
  {
    String quote = String( 1, '\"' );
    return quote + s + quote;
  }

  static void ExpectCharacter( char c, char expected, Errors& errors )
  {
    if( c == expected )
      return;
    const String errorMsg = "Unexpected character " + String( 1, c ) + ", expected " + String( 1, expected );
    TAC_RAISE_ERROR( errorMsg, errors );
  }


  void ParseObject( Json* json, ParseData* parseData, Errors& errors );

  void ParseArray( Json* json, ParseData* parseData, Errors& errors );

  static void ParseQuotedString( StringView* stringView, ParseData* parseData, Errors& errors )
  {
    if( parseData->GetRemainingByteCount() < 2 )
      TAC_RAISE_ERROR( "not enough space for quoted string", errors );
    const char* quotedStrBegin = parseData->GetPos();
    if( *quotedStrBegin != '\"' )
      TAC_RAISE_ERROR( "string does not begin with quote", errors );
    parseData->EatByte();
    if( !parseData->EatUntilCharIsPrev( '\"' ) )
      TAC_RAISE_ERROR( "string does not end with quote", errors );
    const char* quotedStrEnd = parseData->GetPos();
    *stringView = StringView( quotedStrBegin + 1, quotedStrEnd - 1 );
  }

  static void ParseUnknownType( Json* json, ParseData* parseData, Errors& errors )
  {
    json->Clear();
    parseData->EatWhitespace();
    const char* b = parseData->PeekByte();
    TAC_HANDLE_ERROR_IF( !b, "Failed to parse unknown type", errors );
    const char c = *b;
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
      ParseQuotedString( &s, parseData, errors );
      TAC_HANDLE_ERROR( errors );
      *json = Json( s );
    }
    else if( isdigit( c ) || c == '-' || c == '.' )
    {
      auto f = parseData->EatFloat();
      TAC_HANDLE_ERROR_IF( !f.HasValue(), "Failed to parse number", errors );
      *json = Json( ( JsonNumber )f.GetValue() );
    }
    else if( c == 'n' )
    {
      if( !parseData->EatStringExpected( "null" ) )
        TAC_RAISE_ERROR( "Failed parsing null", errors );
      json->mType = JsonType::Null;
    }
    else if( c == 't' )
    {
      const StringView str = "true";
      if( !parseData->EatStringExpected( str ) )
        TAC_RAISE_ERROR( "encountered t but no true", errors );
      *json = Json( true );
    }
    else if( c == 'f' )
    {
      const StringView str = "false";
      if( !parseData->EatStringExpected( str ) )
        TAC_RAISE_ERROR( "encountered f but no false", errors );
      *json = Json( false );
    }
    else if( c == '/' )
    {
      parseData->EatRestOfLine();
      ParseUnknownType( json, parseData, errors );
    }
    else
    {
      const String errorMsg = String( "Unexpected character: " ) + c;
      TAC_RAISE_ERROR( errorMsg, errors );
    }
  }

  static void ParseObject( Json* json, ParseData* parseData, Errors& errors )
  {
    if( parseData->EatWord() != "{" )
      TAC_RAISE_ERROR( "Expected {", errors );

    std::map< String, Json* > children;
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
      ParseQuotedString( &key, parseData, errors );
      TAC_HANDLE_ERROR( errors );
      TAC_HANDLE_ERROR_IF( key.empty(), "object key cannot be empty", errors );
      if( !parseData->EatUntilCharIsPrev( ':' ) )
        TAC_RAISE_ERROR( "Missing : after json key", errors );
      auto child = TAC_NEW Json;
      ParseUnknownType( child, parseData, errors );
      TAC_HANDLE_ERROR( errors );
      children[ key ] = child;
    }

    json->mType = JsonType::Object;
    json->mChildren = children;
  }

  static void ParseArray( Json* json, ParseData* parseData, Errors& errors )
  {
    Vector< Json* > elements;
    parseData->EatWhitespace();
    if( parseData->EatWord() != "[" )
      TAC_RAISE_ERROR( "Expected [", errors );
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

      auto child = TAC_NEW Json;
      ParseUnknownType( child, parseData, errors );
      TAC_HANDLE_ERROR( errors );

      elements.push_back( child );
    }

    json->mType = JsonType::Array;
    json->mElements = elements;
  }

  Json::Json()
  {
    // Why?
    //
    // I think it's like when you default open a json file,
    // it contains a json object
    mType = JsonType::Object;
  }
  Json::Json( StringView s ) { mType = JsonType::String; mString = s; }
  Json::Json( const Json& other )
  {
    *this = other;
  }
  Json::Json( JsonNumber number ) { mType = JsonType::Number; mNumber = number; }
  Json::Json( int number ) { mType = JsonType::Number; mNumber = ( JsonNumber )number; }
  Json::Json( bool b ) { mType = JsonType::Bool; mBoolean = b; }
  Json::~Json()
  {
    Clear();
  }
  void Json::Clear()
  {
    for( auto pair : mChildren )
      delete pair.second;
    mChildren.clear();

    for( auto element : mElements )
      delete element;
    mElements.clear();
  }
  String Json::Stringify( Indentation* indentation ) const
  {
    int iChild = 0;
    String result;
    auto GetSeparator = [ & ]( int childCount ) { return iChild++ != childCount - 1 ? "," : ""; };
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
        result += indentation->ToString() + "{\n";
        indentation->tabCount++;
        for( auto pair : mChildren )
        {
          String childKey = pair.first;
          Json* childValue = pair.second;

          result += indentation->ToString() + DoubleQuote( childKey ) + ":";
          result += Contains( { JsonType::Array, JsonType::Object }, childValue->mType ) ? "\n" : " ";
          result += childValue->Stringify( indentation );
          result += GetSeparator( ( int )mChildren.size() );
          result += "\n";
        }
        indentation->tabCount--;
        result += indentation->ToString() + "}";
      } break;
      case JsonType::Array:
      {
        result += indentation->ToString() + "[\n";
        indentation->tabCount++;
        for( Json* element : mElements )
        {
          if( !Contains( { JsonType::Array, JsonType::Object }, element->mType ) )
            result += indentation->ToString();
          result +=
            element->Stringify( indentation ) +
            GetSeparator( ( int )mElements.size() ) +
            "\n";
        }
        indentation->tabCount--;
        result += indentation->ToString() + "]";
      } break;
    }
    return result;
  }
  String Json::Stringify() const
  {
    Indentation indentation;
    return Stringify( &indentation );
  }

  void Json::Parse( const char* bytes, int byteCount, Errors& errors )
  {
    ParseData parseData( bytes, byteCount );
    ParseUnknownType( this, &parseData, errors );
  }
  void Json::Parse( StringView s, Errors& errors )
  {
    Parse( s.data(), s.size(), errors );
  }


  Json& Json::GetChild( StringView key )
  {
    Json* child = mChildren[ key ];
    if( child )
      return *child;
    child = TAC_NEW Json;
    child->mType = JsonType::Null;
    mChildren[ key ] = child;
    mType = JsonType::Object;
    return *child;
  }
  Json& Json::operator[]( StringView key )
  {
    return GetChild( key );
  }
  Json& Json::operator[]( const char* key )
  {
    return GetChild( key );
  }
  void Json::operator = ( const Json* json )
  {
    *this = *json;
  }
  void Json::operator = ( const Json& json )
  {
    mType = json.mType;
    mString = json.mString;
    mNumber = json.mNumber;
    mBoolean = json.mBoolean;
    for( auto pair : json.mChildren )
    {
      String key = pair.first;
      Json* value = pair.second;
      auto childCopy = TAC_NEW Json;
      *childCopy = *value;
      mChildren[ key ] = childCopy;
    }
    for( const Json* child : json.mElements )
    {
      auto childCopy = TAC_NEW Json;
      *childCopy = *child;
      mElements.push_back( childCopy );
    }
  }
  void Json::operator = ( StringView str ) { *this = Json( str ); }
  void Json::operator = ( JsonNumber number ) { *this = Json( number ); }
  void Json::operator = ( int number ) { *this = Json( number ); }
  void Json::operator = ( bool b ) { *this = Json( b ); }
  Json::operator String() { return mString; }
  Json::operator JsonNumber() { return mNumber; }
  Json::operator bool() { return mBoolean; }


  String Indentation::ToString()
  {
    String spacer;
    if( convertTabsToSpaces )
      for( int iSpace = 0; iSpace < spacesPerTab; ++iSpace )
        spacer += " ";
    else
      spacer = "\t";

    String result;
    for( int iTab = 0; iTab < tabCount; ++iTab )
      result += spacer;
    return result;
  }
}
