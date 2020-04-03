#include "tacJson.h"
#include "tacAlgorithm.h"



static TacString CharToString( char c )
{
  return TacString( 1, c );
}
static TacString Surround( const TacString& inner, const TacString& outer )
{
  return outer + inner + outer;
}
static TacString DoubleQuote( const TacString& s )
{
  return Surround( s, CharToString( '\"' ) );
}
static void ExpectCharacter( char c, char expected, TacErrors& errors )
{
  if( c == expected )
    return;
  errors += "Unexpected character " + CharToString( c ) + ", expected " + CharToString( expected );
}

struct TacParseData
{
  const char* mBytes;
  int mByteCount;
  int mIByte;

  void ByteEat(  char& c, TacErrors& errors );
  void BytePeek(  char& c, TacErrors& errors );
  void BytePeekUnchecked(  char& c );
  void ByteIncrement(  int byteCount = 1 );

  void EatRestOfLine(  TacErrors& errors );
  void SkipLeadingWhitespace();

  void ParseNumber(  TacJsonNumber& jsonNumber, TacErrors& errors );
  void ParseString(  TacString& stringToParse, TacErrors& errors );
  void ParseStringExpected(  const TacString& expected, TacErrors& errors );
};

void TacParseData::ByteEat(  char& c, TacErrors& errors )
{
  BytePeek( c, errors );
  ByteIncrement();
}

void TacParseData::BytePeek(  char& c, TacErrors& errors )
{
  if( mIByte >= mByteCount )
  {
    errors = "Expected more bytes";
    return;
  }
  BytePeekUnchecked( c );
}

void TacParseData::BytePeekUnchecked(  char& c )
{
  c = mBytes[ mIByte ];
}

void TacParseData::ByteIncrement( int byteCount )
{
  mIByte += byteCount;
}

void TacParseData::EatRestOfLine( TacErrors& errors )
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

void TacParseData::ParseNumber( TacJsonNumber& jsonNumber, TacErrors& errors )
{
  SkipLeadingWhitespace();
  TacString s;
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

void TacParseData::ParseString(  TacString& stringToParse, TacErrors& errors )
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

void TacParseData::ParseStringExpected(  const TacString& expected, TacErrors& errors )
{
  int expectedByteCount = expected.size();
  int remainingByteCount = mByteCount - mIByte;
  if( remainingByteCount < expectedByteCount )
  {
    errors = "Not enough bytes";
    TAC_HANDLE_ERROR( errors );
  }
  TacString actual( mBytes + mIByte, expectedByteCount );
  ByteIncrement( expectedByteCount );
  if( actual != expected )
  {
    errors = "Expected " + expected + ", actual " + actual;
    TAC_HANDLE_ERROR( errors );
  }
}

void TacParseData::SkipLeadingWhitespace()
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

void ParseObject( TacJson* json, TacParseData* parseData, TacErrors& errors );
void ParseArray( TacJson* json, TacParseData* parseData, TacErrors& errors );

static void ParseUnknownType( TacJson* json, TacParseData* parseData, TacErrors& errors )
{
  char c;
  json->Clear();
  parseData->SkipLeadingWhitespace( );
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
    TacString s;
    parseData->ParseString( s, errors );
    TAC_HANDLE_ERROR( errors );
    *json = TacJson( s );
  }
  else if( isdigit( c ) || c == '-' || c == '.' )
  {
    TacJsonNumber number;
    parseData->ParseNumber( number, errors );
    TAC_HANDLE_ERROR( errors );
    *json = TacJson( number );
  }
  else if( c == 'n' )
  {
    parseData->ParseStringExpected( "null", errors );
    TAC_HANDLE_ERROR( errors );
    json->mType = TacJsonType::Null;
  }
  else if( c == 't' )
  {
    parseData->ParseStringExpected( "true", errors );
    TAC_HANDLE_ERROR( errors );
    *json = TacJson( true );
  }
  else if( c == 'f' )
  {
    parseData->ParseStringExpected( "false", errors );
    TAC_HANDLE_ERROR( errors );
    *json = TacJson( false );
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

static void ParseObject( TacJson* json, TacParseData* parseData, TacErrors& errors )
{
  char c;
  parseData->SkipLeadingWhitespace();
  parseData->ByteEat( c, errors );
  TAC_HANDLE_ERROR( errors );
  ExpectCharacter( c, '{', errors );
  TAC_HANDLE_ERROR( errors );
  TacString key;
  enum class TacParseObjectStep
  {
    Key,
    Colon,
    ValuePre,
    ValuePost
  };

  std::map< TacString, TacJson* > children;
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
    auto child = new TacJson();
    ParseUnknownType( child, parseData, errors );
    TAC_HANDLE_ERROR( errors );
    children[ key ] = child;
  }

  json->mType = TacJsonType::Object;
  json->mChildren = children;
}

static void ParseArray( TacJson* json, TacParseData* parseData, TacErrors& errors )
{
  char c;
  TacVector< TacJson* > elements;
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
    auto child = new TacJson();
    ParseUnknownType( child, parseData, errors );
    TAC_HANDLE_ERROR( errors );

    elements.push_back( child );
  }

  json->mType = TacJsonType::Array;
  json->mElements = elements;
}

TacJson::TacJson()
{
  // Why?
  //
  // I think it's like when you default open a json file,
  // it contains a json object
  mType = TacJsonType::Object;
}
TacJson::TacJson( const char* str ) { mType = TacJsonType::String; mString = str; }
TacJson::TacJson( const TacString& s ) { mType = TacJsonType::String; mString = s; }
TacJson::TacJson( const TacJson& other )
{
  *this = other;
}
TacJson::TacJson( TacJsonNumber number ) { mType = TacJsonType::Number; mNumber = number; }
TacJson::TacJson( int number ) { mType = TacJsonType::Number; mNumber = ( TacJsonNumber )number; }
TacJson::TacJson( bool b ) { mType = TacJsonType::Bool; mBoolean = b; }
TacJson::~TacJson()
{
  Clear();
}
void TacJson::Clear()
{
  for( auto pair : mChildren )
    delete pair.second;
  mChildren.clear();

  for( auto element : mElements )
    delete element;
  mElements.clear();
}
TacString TacJson::Stringify( TacIndentation* indentation ) const
{
  int iChild = 0;
  TacString result;
  auto GetSeparator = [ & ]( int childCount ) { return iChild++ != childCount - 1 ? "," : ""; };
  switch( mType )
  {
    case TacJsonType::String: result = DoubleQuote( mString ); break;
    case TacJsonType::Number:
    {
      if( ( ( TacJsonNumber )( ( int )mNumber ) ) == mNumber )
        result = TacToString( ( int )mNumber );
      else
        result = TacToString( mNumber );
    } break;
    case TacJsonType::Null: result = "null"; break;
    case TacJsonType::Bool: result = mBoolean ? "true" : "false"; break;
    case TacJsonType::Object:
    {
      result += indentation->ToString() + "{\n";
      indentation->tabCount++;
      for( auto pair : mChildren )
      {
        TacString childKey = pair.first;
        TacJson* childValue = pair.second;

        result += indentation->ToString() + DoubleQuote( childKey ) + ":";
        result += TacContains( { TacJsonType::Array, TacJsonType::Object }, childValue->mType ) ? "\n" : " ";
        result += childValue->Stringify( indentation );
        result += GetSeparator( ( int )mChildren.size() );
        result += "\n";
      }
      indentation->tabCount--;
      result += indentation->ToString() + "}";
    } break;
    case TacJsonType::Array:
    {
      result += indentation->ToString() + "[\n";
      indentation->tabCount++;
      for( TacJson* element : mElements )
      {
        if( !TacContains( { TacJsonType::Array, TacJsonType::Object }, element->mType ) )
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
TacString TacJson::Stringify() const
{
  TacIndentation indentation;
  return Stringify( &indentation );
}

void TacJson::Parse( const char* bytes, int byteCount, TacErrors& errors )
{
  TacParseData parseData = { bytes, byteCount, 0 };
  ParseUnknownType( this, &parseData, errors );
}
void TacJson::Parse( const TacString& s, TacErrors& errors )
{
  Parse( s.data(), s.size(), errors );
}


TacJson& TacJson::GetChild( TacStringView key )
{
  TacJson* child = mChildren[ key ];
  if( child )
    return *child;
  child = new TacJson();
  child->mType = TacJsonType::Null;
  mChildren[ key ] = child;
  mType = TacJsonType::Object;
  return *child;
}
TacJson& TacJson::operator[]( const char* key )
{
  return GetChild( key );
}
TacJson& TacJson::operator[]( const TacString& key )
{
  return GetChild( key );
}
void TacJson::operator = ( const TacJson* json )
{
  *this = *json;
}
void TacJson::operator = ( const TacJson& json )
{
  mType = json.mType;
  mString = json.mString;
  mNumber = json.mNumber;
  mBoolean = json.mBoolean;
  for( auto pair : json.mChildren )
  {
    TacString key = pair.first;
    TacJson* value = pair.second;
    auto childCopy = new TacJson();
    *childCopy = *value;
    mChildren[ key ] = childCopy;
  }
  for( const TacJson* child : json.mElements )
  {
    auto childCopy = new TacJson();
    *childCopy = *child;
    mElements.push_back( childCopy );
  }
}
void TacJson::operator = ( const char* str ){ *this = TacJson( str ); }
void TacJson::operator = ( const TacString& str ){ *this = TacJson( str ); }
void TacJson::operator = ( TacJsonNumber number ){ *this = TacJson( number ); }
void TacJson::operator = ( int number ){ *this = TacJson( number ); }
void TacJson::operator = ( bool b ) { *this = TacJson( b ); }
TacJson::operator TacString() { return mString; }
TacJson::operator TacJsonNumber() { return mNumber; }
TacJson::operator bool() { return mBoolean; }


TacString TacIndentation::ToString()
{
  TacString spacer;
  if( convertTabsToSpaces )
    for( int iSpace = 0; iSpace < spacesPerTab; ++iSpace )
      spacer += " ";
  else
    spacer = "\t";

  TacString result;
  for( int iTab = 0; iTab < tabCount; ++iTab )
    result += spacer;
  return result;
}
