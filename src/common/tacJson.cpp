#include "src/common/tacJson.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacMemory.h"

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
    errors += "Unexpected character " + String( 1, c ) + ", expected " + String( 1, expected );
  }

  struct ParseData
  {
    const char* mBytes;
    int mByteCount;
    int mIByte;

    void ByteEat( char& c, Errors& errors );
    void BytePeek( char& c, Errors& errors );
    void BytePeekUnchecked( char& c );
    void ByteIncrement( int byteCount = 1 );

    void EatRestOfLine( Errors& errors );
    void SkipLeadingWhitespace();

    void ParseNumber( JsonNumber& jsonNumber, Errors& errors );
    void ParseString( String& stringToParse, Errors& errors );
    void ParseStringExpected( StringView expected, Errors& errors );
  };

  void ParseData::ByteEat( char& c, Errors& errors )
  {
    BytePeek( c, errors );
    ByteIncrement();
  }

  void ParseData::BytePeek( char& c, Errors& errors )
  {
    if( mIByte >= mByteCount )
    {
      errors = "Expected more bytes";
      return;
    }
    BytePeekUnchecked( c );
  }

  void ParseData::BytePeekUnchecked( char& c )
  {
    c = mBytes[ mIByte ];
  }

  void ParseData::ByteIncrement( int byteCount )
  {
    mIByte += byteCount;
  }

  void ParseData::EatRestOfLine( Errors& errors )
  {
    for( ;; )
    {
      char c;
      ByteEat( c, errors );
      TAC_HANDLE_ERROR( errors );
      if( c == '\n' )
        return;
    }
  }

  void ParseData::ParseNumber( JsonNumber& jsonNumber, Errors& errors )
  {
    SkipLeadingWhitespace();
    String s;
    char c;
    while( mIByte < mByteCount )
    {
      BytePeekUnchecked( c );
      bool validCharater = isdigit( c )
        || c == '.'
        || c == 'e'
        || c == 'E'
        || c == '+'
        || c == '-';
      if( !validCharater )
        break;
      s += c;
      ByteIncrement();
    }
    if( s.empty() )
    {
      errors = "No digits";
      return;
    }
    jsonNumber = atof( s.c_str() );
  }

  void ParseData::ParseString( String& stringToParse, Errors& errors )
  {
    char c;
    SkipLeadingWhitespace();
    ByteEat( c, errors );
    TAC_HANDLE_ERROR( errors );
    ExpectCharacter( c, '\"', errors );
    TAC_HANDLE_ERROR( errors );
    stringToParse.clear();
    for( ;; )
    {
      ByteEat( c, errors );
      TAC_HANDLE_ERROR( errors );
      if( c == '\"' )
        return;
      stringToParse += c;
    }
  }

  void ParseData::ParseStringExpected( StringView expected, Errors& errors )
  {
    int expectedByteCount = expected.size();
    int remainingByteCount = mByteCount - mIByte;
    if( remainingByteCount < expectedByteCount )
    {
      errors = "Not enough bytes";
      TAC_HANDLE_ERROR( errors );
    }
    String actual( mBytes + mIByte, expectedByteCount );
    ByteIncrement( expectedByteCount );
    if( actual != expected )
    {
      errors = "Expected " + expected + ", actual " + actual;
      TAC_HANDLE_ERROR( errors );
    }
  }

  void ParseData::SkipLeadingWhitespace()
  {
    char c;
    while( mIByte < mByteCount )
    {
      BytePeekUnchecked( c );
      if( !isspace( c ) )
        break;
      ByteIncrement();
    }
  }

  void ParseObject( Json* json, ParseData* parseData, Errors& errors );
  void ParseArray( Json* json, ParseData* parseData, Errors& errors );

  static void ParseUnknownType( Json* json, ParseData* parseData, Errors& errors )
  {
    char c;
    json->Clear();
    parseData->SkipLeadingWhitespace();
    parseData->BytePeek( c, errors );
    TAC_HANDLE_ERROR( errors );
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
      String s;
      parseData->ParseString( s, errors );
      TAC_HANDLE_ERROR( errors );
      *json = Json( s );
    }
    else if( isdigit( c ) || c == '-' || c == '.' )
    {
      JsonNumber number;
      parseData->ParseNumber( number, errors );
      TAC_HANDLE_ERROR( errors );
      *json = Json( number );
    }
    else if( c == 'n' )
    {
      parseData->ParseStringExpected( "null", errors );
      TAC_HANDLE_ERROR( errors );
      json->mType = JsonType::Null;
    }
    else if( c == 't' )
    {
      parseData->ParseStringExpected( "true", errors );
      TAC_HANDLE_ERROR( errors );
      *json = Json( true );
    }
    else if( c == 'f' )
    {
      parseData->ParseStringExpected( "false", errors );
      TAC_HANDLE_ERROR( errors );
      *json = Json( false );
    }
    else if( c == '/' )
    {
      parseData->EatRestOfLine( errors );
      TAC_HANDLE_ERROR( errors );
      ParseUnknownType( json, parseData, errors );
    }
    else
    {
      errors = "Unexpected character: " + c;
    }
  }

  static void ParseObject( Json* json, ParseData* parseData, Errors& errors )
  {
    char c;
    parseData->SkipLeadingWhitespace();
    parseData->ByteEat( c, errors );
    TAC_HANDLE_ERROR( errors );
    ExpectCharacter( c, '{', errors );
    TAC_HANDLE_ERROR( errors );
    String key;
    enum class ParseObjectStep
    {
      Key,
      Colon,
      ValuePre,
      ValuePost
    };

    std::map< String, Json* > children;
    for( ;; )
    {
      parseData->SkipLeadingWhitespace();
      parseData->BytePeek( c, errors );
      TAC_HANDLE_ERROR( errors );
      if( c == '}' )
      {
        parseData->ByteIncrement();
        break;
      }
      if( c == ',' )
      {
        parseData->ByteIncrement();
        continue;
      }
      if( c == '/' )
      {
        parseData->EatRestOfLine( errors );
        TAC_HANDLE_ERROR( errors );
        continue;
      }
      ExpectCharacter( c, '\"', errors );
      TAC_HANDLE_ERROR( errors );
      parseData->ParseString( key, errors );
      TAC_HANDLE_ERROR( errors );
      if( key.empty() )
      {
        errors = "Empty key";
        TAC_HANDLE_ERROR( errors );
      }
      parseData->SkipLeadingWhitespace();
      parseData->ByteEat( c, errors );
      TAC_HANDLE_ERROR( errors );
      ExpectCharacter( c, ':', errors );
      TAC_HANDLE_ERROR( errors );
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
    char c;
    Vector< Json* > elements;
    parseData->SkipLeadingWhitespace();
    parseData->ByteEat( c, errors );
    TAC_HANDLE_ERROR( errors );
    ExpectCharacter( c, '[', errors );
    TAC_HANDLE_ERROR( errors );
    for( ;; )
    {
      parseData->SkipLeadingWhitespace();
      parseData->BytePeek( c, errors );
      TAC_HANDLE_ERROR( errors );
      if( c == ']' )
      {
        parseData->ByteIncrement();
        break;
      }
      if( c == ',' )
      {
        parseData->ByteIncrement();
        continue;
      }
      if( '/' == c )
      {
        parseData->EatRestOfLine( errors );
        TAC_HANDLE_ERROR( errors );
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
    ParseData parseData = { bytes, byteCount, 0 };
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
