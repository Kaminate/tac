// This file deals with localizing to different languages
//
// The way localization works is that a string reference is
// used to refer to phrase in any given language.
//
// The codepoints in a phrase are encoded into a utf-8 string

#pragma once

//#include "common/tacPreprocessor.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"
#include <map>

enum class TacLanguage
{
  Arabic,
  Chinese,
  English,
  Japanese,
  Korean,
  Russian,
  Spanish,
  Count
};

const TacString TacLanguages[ ( int )TacLanguage::Count ] =
{
  "Arabic",
  "Chinese",
  "English",
  "Japanese",
  "Korean",
  "Russian",
  "Spanish",
};

inline const TacString& TacLanguageToStr( TacLanguage language )
{
  return TacLanguages[ ( int )language ];
}
inline TacLanguage TacGetLanguage( const TacString& str )
{
  for( int i = 0; i < ( int )TacLanguage::Count; ++i )
    if( TacLanguages[ i ] == str )
      return ( TacLanguage )i;
  return TacLanguage::Count;
}
void TacLanguageDebugImgui( const TacString& name, TacLanguage* language );

typedef uint32_t TacCodepoint;

bool TacIsAsciiCharacter( TacCodepoint codepoint, char c );

struct TacUTF8Converter
{
  static void Convert(
    const TacString& text,
    TacVector< TacCodepoint >& codepoints,
    TacErrors& errors );
  void Run( TacVector< TacCodepoint >& codepoints, TacErrors& errors );
  void TacIterateUTF8( TacCodepoint* codepoint, TacErrors& errors );
  char GetNextByte( TacErrors& errors );
  const char* mBegin = nullptr;
  const char* mEnd = nullptr;
};

struct TacLocalizedStringStuff
{
  // TODO: don't bother storing the codepoints, just compute them on the fly
  TacVector< TacCodepoint > mCodepoints;
  TacString mUTF8String;
};

struct TacLocalizedString
{
  TacString mReference;
  std::map< TacLanguage, TacLocalizedStringStuff > mCodepoints;
};

struct TacLocalization
{
  const TacVector< TacCodepoint >& GetString( TacLanguage language, const TacString& reference );
  void Load( const TacString& path, TacErrors& errors );
  bool EatWhitespace();
  bool EatNewLine();
  TacString EatWord();
  void DebugImgui();

  char* mBegin = nullptr;
  char* mEnd = nullptr;
  TacVector< char > mBytes;
  TacVector< TacLocalizedString > mLocalizedStrings;
};

