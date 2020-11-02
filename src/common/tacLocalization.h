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
  typedef uint32_t Codepoint;

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

  StringView LanguageToStr( Language );
  Language   GetLanguage( StringView );
  void       LanguageDebugImgui( StringView name, Language* language );
  bool       IsAsciiCharacter( Codepoint );

  struct CodepointView
  {
    CodepointView() = default;
    CodepointView( const Codepoint*, int );
    Codepoint        operator[]( int i ) const;
    const Codepoint* data() const;
    const Codepoint* begin() const;
    const Codepoint* end() const;
    int              size() const;
    bool             empty() const;
    const Codepoint* mCodepoints = nullptr;
    int              mCodepointCount = 0;
  };

  bool operator == ( CodepointView, CodepointView );
  bool operator != ( CodepointView, CodepointView );

  CodepointView UTF8ToCodepoints( StringView );
  StringView CodepointsToUTF8( CodepointView );


  struct LocalizedStringStuff
  {
    void                SetCodepoints( CodepointView );
    // TODO: don't bother storing the codepoints, just compute them on the fly
    Vector< Codepoint > mCodepoints;
    String              mUTF8String;
  };

  typedef std::map< Language, LocalizedStringStuff > LanguageMap;

  struct LocalizedString
  {
    String               mReference;
    LanguageMap          mCodepoints;
  };

  struct Localization
  {
    const Vector< Codepoint >& GetString( Language language, StringView reference );
    void                       Load( StringView path, Errors& );
    void                       DebugImgui();
    Vector< LocalizedString >  mLocalizedStrings;
  };

  extern Localization gLocalization;

}
