// This file deals with localizing to different languages
//
// The way localization works is that a string reference is
// used to refer to phrase in any given language.
//
// The codepoints in a phrase are encoded into a utf-8 string

#pragma once

//#include "src/common/Preprocessor.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include <map>

namespace Tac
{


  enum class Language
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

  const String Languages[ ( int )Language::Count ] =
  {
    "Arabic",
    "Chinese",
    "English",
    "Japanese",
    "Korean",
    "Russian",
    "Spanish",
  };

  inline StringView LanguageToStr( Language language )
  {
    return Languages[ ( int )language ];
  }
  inline Language GetLanguage( StringView str )
  {
    for( int i = 0; i < ( int )Language::Count; ++i )
      if( Languages[ i ] == str )
        return ( Language )i;
    return Language::Count;
  }
  void LanguageDebugImgui( StringView name, Language* language );

  typedef uint32_t Codepoint;

  bool IsAsciiCharacter( Codepoint codepoint );


  struct CodepointView
  {
    CodepointView() = default;
    CodepointView( const Codepoint*, int );
    Codepoint operator[]( int i ) const;
    const Codepoint* data() const;
    const Codepoint* begin() const;
    const Codepoint* end() const;
    int size() const;
    bool empty() const;
    const Codepoint* mCodepoints = nullptr;
    int mCodepointCount = 0;
  };

  bool operator == ( CodepointView a, CodepointView b );
  bool operator != ( CodepointView a, CodepointView b );

  CodepointView UTF8ToCodepoints( StringView );
  StringView CodepointsToUTF8( CodepointView );


  struct LocalizedStringStuff
  {
    void SetCodepoints( CodepointView codepoints );
    // TODO: don't bother storing the codepoints, just compute them on the fly
    Vector< Codepoint > mCodepoints;
    String mUTF8String;
  };

  struct LocalizedString
  {
    String mReference;
    std::map< Language, LocalizedStringStuff > mCodepoints;
  };

  struct Localization
  {
    Localization();
    static Localization* Instance;
    const Vector< Codepoint >& GetString( Language language, StringView reference );
    void Load( StringView path, Errors& errors );
    bool EatWhitespace();
    bool EatNewLine();
    String EatWord();
    void DebugImgui();

    char* mBegin = nullptr;
    char* mEnd = nullptr;
    Vector< char > mBytes;
    Vector< LocalizedString > mLocalizedStrings;
  };

}
