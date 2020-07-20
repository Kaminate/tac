#include "src/common/tacLocalization.h"
#include "src/common/tacUtility.h"
#include "src/common/tacMemory.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacTemporaryMemory.h"

namespace Tac
{


bool IsAsciiCharacter( Codepoint codepoint )
{
  return codepoint < ( Codepoint )128;
}

char UTF8Converter::GetNextByte( Errors& errors )
{
  if( mBegin >= mEnd )
  {
    errors = "utf8 get next byte failed";
    return 0;
  }
  return *mBegin++;
}
void UTF8Converter::IterateUTF8( Codepoint* codepoint, Errors& errors )
{
  char b0 = GetNextByte( errors );
  TAC_HANDLE_ERROR( errors );
  if( ( b0 & ( 1 << 7 ) ) == 0 )
  {
    *codepoint = ( 0b01111111 & b0 ) << 0;
    return;
  }
  char b1 = GetNextByte( errors );
  TAC_HANDLE_ERROR( errors );
  if( ( b0 & ( 1 << 5 ) ) == 0 )
  {
    *codepoint =
      ( ( 0b00111111 & b1 ) << 0 ) |
      ( ( 0b00011111 & b0 ) << 6 );
    return;
  }
  char b2 = GetNextByte( errors );
  TAC_HANDLE_ERROR( errors );
  if( ( b0 & ( 1 << 4 ) ) == 0 )
  {
    *codepoint =
      ( ( 0b00111111 & b2 ) << 0 ) |
      ( ( 0b00111111 & b1 ) << 6 ) |
      ( ( 0b00001111 & b0 ) << 12 );
    return;
  }
  char b3 = GetNextByte( errors );
  TAC_HANDLE_ERROR( errors );
  *codepoint =
    ( ( 0b00111111 & b3 ) << 0 ) |
    ( ( 0b00111111 & b2 ) << 6 ) |
    ( ( 0b00111111 & b1 ) << 12 ) |
    ( ( 0b00000111 & b0 ) << 18 );
}
void UTF8Converter::Run( Vector< Codepoint >& codepoints, Errors& errors )
{
  while( mBegin < mEnd )
  {
    Codepoint codepoint;
    IterateUTF8( &codepoint, errors );
    TAC_HANDLE_ERROR( errors );
    codepoints.push_back( codepoint );
  }
}
void UTF8Converter::Convert(
  const String& text,
  Vector< Codepoint >& codepoints,
  Errors& errors )
{
  UTF8Converter converter;
  converter.mBegin = text.data();
  converter.mEnd = text.data() + text.size();
  converter.Run( codepoints, errors );
}
void UTF8Converter::Convert(
  const Vector< Codepoint >& codepoints,
  String& text )
{
  for( Codepoint codepoint : codepoints )
  {
    if( codepoint >= 0x10000 )
    {
      text.push_back( 0b11110000 | ( 0b00000111 & ( codepoint >> 18 ) ) );
      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 12 ) ) );
      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 6 ) ) );
      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 0 ) ) );
    }
    else if( codepoint >= 0x800 )
    {
      text.push_back( 0b11100000 | ( 0b00001111 & ( codepoint >> 12 ) ) );
      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 6 ) ) );
      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 0 ) ) );
    }
    else if( codepoint > 0x80 )
    {
      text.push_back( 0b11000000 | ( 0b00011111 & ( codepoint >> 6 ) ) );
      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 0 ) ) );
    }
    else
    {
      text.push_back( 0b01111111 & codepoint );
    }
  }
}
String Localization::EatWord()
{
  String result;
  while( mBegin < mEnd )
  {
    if( ( EatWhitespace() || EatNewLine() ) && !result.empty() )
      break;
    result += *mBegin++;
  }
  return result;
}

const Vector< Codepoint >& Localization::GetString( Language language, const String& reference )
{
  for( const auto& localizedString : mLocalizedStrings )
  {
    if( localizedString.mReference != reference )
      continue;
    return localizedString.mCodepoints.at( language ).mCodepoints;
  }
  TAC_INVALID_CODE_PATH;
  static Vector< Codepoint > result;
  return result;
}

Localization* Localization::Instance = nullptr;
Localization::Localization()
{
  Instance = this;
}
void Localization::Load( const String& path, Errors& errors )
{
  mBytes = TemporaryMemoryFromFile( path, errors );
  TAC_HANDLE_ERROR( errors );
  mBegin = mBytes.data();
  mEnd = mBytes.data() + mBytes.size();

  String reference;
  while( mBegin < mEnd )
  {
    if( reference.empty() )
      reference = EatWord();
    LocalizedString localizedString;
    localizedString.mReference = reference;
    while( mBegin < mEnd )
    {
      auto word = EatWord();
      auto language = GetLanguage( word );
      if( language == Language::Count )
      {
        reference = word;
        break;
      }
      EatWhitespace();
      UTF8Converter converter;
      converter.mBegin = mBegin;
      converter.mEnd = mBegin;
      while( mBegin < mEnd )
      {
        converter.mEnd = mBegin;
        if( EatNewLine() )
          break;
        mBegin++;
      }
      String utf8String( converter.mBegin, converter.mEnd );

      Vector< Codepoint > codepoints;
      converter.Run( codepoints, errors );
      if( !errors.empty() )
      {
        errors += "Failed reading " + reference + " of " + word;
        return;
      }
      LocalizedStringStuff localizedStringStuff;
      localizedStringStuff.mCodepoints = codepoints;
      localizedStringStuff.mUTF8String = utf8String;
      localizedString.mCodepoints[ language ] = localizedStringStuff;
    }
    mLocalizedStrings.push_back( localizedString );
  }
  mBytes.clear();
  mBegin = nullptr;
  mEnd = nullptr;
}
bool Localization::EatNewLine()
{
  auto oldBegin = mBegin;
  if( mBegin < mEnd &&
    mBegin[ 0 ] == '\n' )
  {
    mBegin++;
  }
  if( mBegin + 1 < mEnd &&
    mBegin[ 0 ] == '\r' &&
    mBegin[ 1 ] == '\n' )
  {
    mBegin += 2;
  }
  return oldBegin < mBegin;
}
bool Localization::EatWhitespace()
{
  auto oldBegin = mBegin;
  while( mBegin < mEnd )
  {
    if( !Contains( { ' ', '\t' }, *mBegin ) )
      break;
    mBegin++;
  }
  return oldBegin < mBegin;
}


void Localization::DebugImgui()
{
  //if( !ImGui::CollapsingHeader( "Localization" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //for( auto & localizedStrings : mLocalizedStrings )
  //{
  //  if( !ImGui::CollapsingHeader( localizedStrings.mReference.c_str() ) )
  //    continue;
  //  ImGui::Indent();
  //  OnDestruct( ImGui::Unindent() );
  //  for( auto kvp : localizedStrings.mCodepoints )
  //  {
  //    auto language = kvp.first;
  //    auto& languageStr = LanguageToStr( language );
  //    auto& codepoints = kvp.second;
  //    if( !ImGui::CollapsingHeader( languageStr.c_str() ) )
  //      continue;
  //    ImGui::Indent();
  //    OnDestruct( ImGui::Unindent() );
  //    ImGui::InputText( "", codepoints.mUTF8String );
  //  }
  //}
}


// Should I make an ImGui::Enum?
void LanguageDebugImgui( const String& name, Language* language )
{
  //auto currentItem = ( int )( *language );
  //auto itemGetter = []( void* data, int idx, const char** outText )
  //{
  //  UnusedParameter( data );
  //  *outText = LanguageToStr( ( Language )idx ).c_str();
  //  return true;
  //};
  //if( !ImGui::Combo( name.c_str(), &currentItem, itemGetter, nullptr, ( int )Language::Count ) )
  //  return;
  //*language = ( Language )currentItem;
}
}
