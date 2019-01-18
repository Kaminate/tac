#include "tacJson.h"
#include "tacAlgorithm.h"

TacJson::TacJson()
{
  // Why?
  mType = TacJsonType::Object;
}
//TacJson::TacJson( const char* str )
//{
//  mType = TacJsonType::String;
//  mString = str;
//}
//TacJson::TacJson( const TacString& s )
//{
//  mType = TacJsonType::String;
//  mString = s;
//}
//TacJson::TacJson( TacJsonNumber number )
//{
//  mType = TacJsonType::Number;
//  mNumber = number;
//}
//TacJson::TacJson( bool b )
//{
//  mType = TacJsonType::Bool;
//  mBoolean = b;
//}
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
TacString TacJson::Stringify( TacStringifyData* stringifyData ) const
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
    result += stringifyData->ToString() + "{\n";
    stringifyData->tabCount++;
    for( auto pair : mChildren )
    {
      TacString childKey = pair.first;
      TacJson* childValue = pair.second;

      result += stringifyData->ToString() + DoubleQuote( childKey ) + ":";
      result += TacContains( { TacJsonType::Array, TacJsonType::Object }, childValue->mType ) ? "\n" : " ";
      result += childValue->Stringify( stringifyData );
      result += GetSeparator( ( int )mChildren.size() );
      result += "\n";
    }
    stringifyData->tabCount--;
    result += stringifyData->ToString() + "}";
  } break;
  case TacJsonType::Array:
  {
    result += stringifyData->ToString() + "[\n";
    stringifyData->tabCount++;
    for( TacJson* element : mElements )
    {
      result += element->Stringify( stringifyData ) +
        GetSeparator( ( int )mElements.size() ) +
        "\n";
    }
    stringifyData->tabCount--;
    result += stringifyData->ToString() + "]";
  } break;
  }
  return result;
}
TacString TacJson::Stringify() const
{
  TacStringifyData stringifyData;
  return Stringify( &stringifyData );
}
// Make this static?
TacString TacJson::CharToString( char c ) const
{
  return TacString( 1, c );
}
// Make this static?
TacString TacJson::Surround( const TacString& inner, const TacString& outer ) const
{
  return outer + inner + outer;
}
TacString TacJson::DoubleQuote( const TacString& s ) const
{
  return Surround( s, CharToString( '\"' ) );
}
void TacJson::ByteEat( TacParseData* parseData, char& c, TacErrors& errors )
{
  BytePeek( parseData, c, errors );
  ByteIncrement( parseData );
}
void TacJson::BytePeek( TacParseData* parseData, char& c, TacErrors& errors )
{
  if( parseData->mIByte >= parseData->mByteCount )
  {
    errors = "Expected more bytes";
    return;
  }
  BytePeekUnchecked( parseData, c );
}
// Make this static?
void TacJson::BytePeekUnchecked( TacParseData* parseData, char& c )
{
  c = parseData->mBytes[ parseData->mIByte ];
}
// Make this static?
void TacJson::ByteIncrement( TacParseData* parseData, int byteCount )
{
  parseData->mIByte += byteCount;
}
void TacJson::UnexpectedCharacter( char c, TacErrors& errors )
{
  errors += "Unexpected character " + CharToString( c );
}
void TacJson::ExpectCharacter( char c, char expected, TacErrors& errors )
{
  if( c == expected )
    return;
  UnexpectedCharacter( c, errors );
  errors += ", expected " + CharToString( expected );
}
void TacJson::SkipLeadingWhitespace( TacParseData* parseData )
{
  char c;
  while( parseData->mIByte < parseData->mByteCount )
  {
    BytePeekUnchecked( parseData, c );
    if( !isspace( c ) )
      break;
    ByteIncrement( parseData );
  }
}
void TacJson::ParseNumber( TacParseData* parseData, TacJsonNumber& jsonNumber, TacErrors& errors )
{
  SkipLeadingWhitespace( parseData );
  TacString s;
  char c;
  while( parseData->mIByte < parseData->mByteCount )
  {
    BytePeekUnchecked( parseData, c );
    bool validCharater = isdigit( c )
      || c == '.'
      || c == 'e'
      || c == 'E'
      || c == '+'
      || c == '-';
    if( !validCharater )
      break;
    s += c;
    ByteIncrement( parseData );
  }
  if( s.empty() )
  {
    errors = "No digits";
    return;
  }
  jsonNumber = atof( s.c_str() );
}
void TacJson::ParseString( TacParseData* parseData, TacString& stringToParse, TacErrors& errors )
{
  char c;
  SkipLeadingWhitespace( parseData );
  ByteEat( parseData, c, errors );
  TAC_HANDLE_ERROR( errors );
  ExpectCharacter( c, '\"', errors );
  TAC_HANDLE_ERROR( errors );
  stringToParse.clear();
  for( ;; )
  {
    ByteEat( parseData, c, errors );
    TAC_HANDLE_ERROR( errors );
    if( c == '\"' )
      return;
    stringToParse += c;
  }
}
void TacJson::ParseStringExpected( TacParseData* parseData, const TacString& expected, TacErrors& errors )
{
  int expectedByteCount = expected.size();
  int remainingByteCount = parseData->mByteCount - parseData->mIByte;
  if( remainingByteCount < expectedByteCount )
  {
    errors = "Not enough bytes";
    TAC_HANDLE_ERROR( errors );
  }
  TacString actual( parseData->mBytes + parseData->mIByte, expectedByteCount );
  ByteIncrement( parseData, expectedByteCount );
  if( actual != expected )
  {
    errors = "Expected " + expected + ", actual " + actual;
    TAC_HANDLE_ERROR( errors );
  }
}
void TacJson::ParseObject( TacParseData* parseData, TacErrors& errors )
{
  char c;
  SkipLeadingWhitespace( parseData );
  ByteEat( parseData, c, errors );
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
  //TacParseObjectStep parseObjectStep = TacParseObjectStep::Key;
  for( ;; )
  {
    SkipLeadingWhitespace( parseData );
    BytePeek( parseData, c, errors );
    TAC_HANDLE_ERROR( errors );
    if( c == '}' )
    {
      ByteIncrement( parseData );
      return;
    }
    if( c == ',' )
    {
      ByteIncrement( parseData );
      continue;
    }
    ExpectCharacter( c, '\"', errors );
    TAC_HANDLE_ERROR( errors );
    ParseString( parseData, key, errors );
    TAC_HANDLE_ERROR( errors );
    if( key.empty() )
    {
      errors = "Empty key";
      return;
    }
    SkipLeadingWhitespace( parseData );
    ByteEat( parseData, c, errors );
    TAC_HANDLE_ERROR( errors );
    ExpectCharacter( c, ':', errors );
    TAC_HANDLE_ERROR( errors );
    auto child = new TacJson();
    child->ParseUnknownType( parseData, errors );
    TAC_HANDLE_ERROR( errors );
    mChildren[ key ] = child;
  }
}
void TacJson::ParseArray( TacParseData* parseData, TacErrors& errors )
{
  char c;
  mType = TacJsonType::Array;
  SkipLeadingWhitespace( parseData );
  ByteEat( parseData, c, errors );
  TAC_HANDLE_ERROR( errors );
  ExpectCharacter( c, '[', errors );
  TAC_HANDLE_ERROR( errors );
  for( ;; )
  {
    SkipLeadingWhitespace( parseData );
    BytePeek( parseData, c, errors );
    TAC_HANDLE_ERROR( errors );
    if( c == ']' )
    {
      ByteIncrement( parseData );
      return;
    }
    if( c == ',' )
    {
      ByteIncrement( parseData );
      continue;
    }
    auto child = new TacJson();
    child->ParseUnknownType( parseData, errors );
    TAC_HANDLE_ERROR( errors );
    mElements.push_back( child );
  }
}
void TacJson::ParseUnknownType( TacParseData* parseData, TacErrors& errors )
{
  char c;
  Clear();
  SkipLeadingWhitespace( parseData );
  BytePeek( parseData, c, errors );
  TAC_HANDLE_ERROR( errors );
  if( c == '{' )
  {
    ParseObject( parseData, errors );
  }
  else if( c == '[' )
  {
    ParseArray( parseData, errors );
  }
  else if( c == '\"' )
  {
    ParseString( parseData, mString, errors );
    TAC_HANDLE_ERROR( errors );
    mType = TacJsonType::String;
  }
  else if( isdigit( c ) )
  {
    ParseNumber( parseData, mNumber, errors );
    TAC_HANDLE_ERROR( errors );
    mType = TacJsonType::Number;
  }
  else if( c == 'n' )
  {
    ParseStringExpected( parseData, "null", errors );
    TAC_HANDLE_ERROR( errors );
    mType = TacJsonType::Null;
  }
  else if( c == 't' )
  {
    ParseStringExpected( parseData, "true", errors );
    TAC_HANDLE_ERROR( errors );
    mType = TacJsonType::Bool;
    mBoolean = true;
  }
  else if( c == 'f' )
  {
    ParseStringExpected( parseData, "false", errors );
    TAC_HANDLE_ERROR( errors );
    mType = TacJsonType::Bool;
    mBoolean = false;
  }
  else
  {
    UnexpectedCharacter( c, errors );
  }
}
void TacJson::Parse( const char* bytes, int byteCount, TacErrors& errors )
{
  TacParseData parseData = { bytes, byteCount, 0 };
  ParseObject( &parseData, errors );
}
void TacJson::Parse( const TacString& s, TacErrors& errors )
{
  TacParseData parseData = { s.data(), ( int )s.size(), 0 };
  ParseObject( &parseData, errors );
}
TacJson& TacJson::operator[]( const TacString& key )
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


TacString TacJson::TacStringifyData::ToString()
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
