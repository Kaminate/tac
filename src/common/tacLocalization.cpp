#include "tacLocalization.h"
#include "tacUtility.h"
#include "tacMemory.h"
#include "tacAlgorithm.h"

#include "imgui.h"

bool TacIsAsciiCharacter( TacCodepoint codepoint )
{
  return codepoint < ( TacCodepoint )128;
}

char TacUTF8Converter::GetNextByte( TacErrors& errors )
{
  if( mBegin >= mEnd )
  {
    errors = "utf8 get next byte failed";
    return 0;
  }
  return *mBegin++;
}
void TacUTF8Converter::TacIterateUTF8( TacCodepoint* codepoint, TacErrors& errors )
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
void TacUTF8Converter::Run( TacVector< TacCodepoint >& codepoints, TacErrors& errors )
{
  while( mBegin < mEnd )
  {
    TacCodepoint codepoint;
    TacIterateUTF8( &codepoint, errors );
    TAC_HANDLE_ERROR( errors );
    codepoints.push_back( codepoint );
  }
}
void TacUTF8Converter::Convert(
  const TacString& text,
  TacVector< TacCodepoint >& codepoints,
  TacErrors& errors )
{
  TacUTF8Converter converter;
  converter.mBegin = text.data();
  converter.mEnd = text.data() + text.size();
  converter.Run( codepoints, errors );
}

TacString TacLocalization::EatWord()
{
  TacString result;
  while( mBegin < mEnd )
  {
    if( ( EatWhitespace() || EatNewLine() ) && !result.empty())
      break;
    result += *mBegin++;
  }
  return result;
}

const TacVector< TacCodepoint >& TacLocalization::GetString( TacLanguage language, const TacString& reference )
{
  for( const auto& localizedString : mLocalizedStrings )
  {
    if( localizedString.mReference != reference )
      continue;
    return localizedString.mCodepoints.at( language ).mCodepoints;
  }
  TacInvalidCodePath;
  static TacVector< TacCodepoint > result;
  return result;
}

void TacLocalization::Load( const TacString& path, TacErrors& errors )
{
  mBytes = TacTemporaryMemory( path, errors );
  TAC_HANDLE_ERROR( errors );
  mBegin = mBytes.data();
  mEnd = mBytes.data() + mBytes.size();

  TacString reference;
  while( mBegin < mEnd )
  {
    if( reference.empty() )
      reference = EatWord();
    TacLocalizedString localizedString;
    localizedString.mReference = reference;
    while( mBegin < mEnd )
    {
      auto word = EatWord();
      auto language = TacGetLanguage( word );
      if( language == TacLanguage::Count )
      {
        reference = word;
        break;
      }
      EatWhitespace();
      TacUTF8Converter converter;
      converter.mBegin = mBegin;
      converter.mEnd = mBegin;
      while( mBegin < mEnd )
      {
        converter.mEnd = mBegin;
        if( EatNewLine() )
          break;
        mBegin++;
      }
      TacString utf8String( converter.mBegin, converter.mEnd );

      TacVector< TacCodepoint > codepoints;
      converter.Run( codepoints, errors );
      if( !errors.empty() )
      {
        errors += "Failed reading " + reference + " of " + word;
        return;
      }
      TacLocalizedStringStuff localizedStringStuff;
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
bool TacLocalization::EatNewLine()
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
bool TacLocalization::EatWhitespace()
{
  auto oldBegin = mBegin;
  while( mBegin < mEnd )
  {
    if( !TacContains( { ' ', '\t' }, *mBegin ) )
      break;
    mBegin++;
  }
  return oldBegin < mBegin;
}


void TacLocalization::DebugImgui()
{
  if( !ImGui::CollapsingHeader( "Localization" ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  for( auto & localizedStrings : mLocalizedStrings )
  {
    if( !ImGui::CollapsingHeader( localizedStrings.mReference.c_str() ) )
      continue;
    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    for( auto kvp : localizedStrings.mCodepoints )
    {
      auto language = kvp.first;
      auto& languageStr = TacLanguageToStr( language );
      auto& codepoints = kvp.second;
      if( !ImGui::CollapsingHeader( languageStr.c_str() ) )
        continue;
      ImGui::Indent();
      OnDestruct( ImGui::Unindent() );
      ImGui::InputText( "", codepoints.mUTF8String );
    }
  }
}


// Should I make an ImGui::Enum?
void TacLanguageDebugImgui( const TacString& name, TacLanguage* language )
{
  auto currentItem = ( int )( *language );
  auto itemGetter = []( void* data, int idx, const char** outText )
  {
    TacUnusedParameter( data );
    *outText = TacLanguageToStr( ( TacLanguage )idx ).c_str();
    return true;
  };
  if( !ImGui::Combo( name.c_str(), &currentItem, itemGetter, nullptr, ( int )TacLanguage::Count ) )
    return;
  *language = ( TacLanguage )currentItem;
}
