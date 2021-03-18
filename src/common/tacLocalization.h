// This file deals with localizing to different languages
//
// The way localization works is that a string reference is
// used to refer to phrase in any given language.
//
// The codepoints in a phrase are encoded into a utf-8 string

#pragma once

//#include "src/common/Preprocessor.h"
//#include "src/common/string/tacString.h"
//#include "src/common/tacErrorHandling.h"
//#include "src/common/containers/tacVector.h"

#include <cinttypes> // uint32_t

namespace Tac
{
  typedef uint32_t Codepoint;

  struct StringView;
  struct Errors;

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
  StringView    CodepointsToUTF8( CodepointView );




  CodepointView              LocalizationGetString( Language language, StringView reference );
  void                       LocalizationLoad( StringView path, Errors& );
  void                       LocalizationDebugImgui();
}
