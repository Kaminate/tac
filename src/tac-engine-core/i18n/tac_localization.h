// This file deals with localizing to different languages
//
// The way localization works is that a string reference is
// used to refer to phrase in any given language.
//
// The codepoints in a phrase are encoded into a utf-8 string

#pragma once

//#include "tac-std-lib/tac_core.h"

#include "tac-std-lib/tac_ints.h"


namespace Tac{ struct StringView; struct Errors; }
namespace Tac::Filesystem { struct Path; }

namespace Tac
{
  using Codepoint = u32;

  enum class Language : u32
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
  void       LanguageDebugImgui( StringView name, Language* );
  bool       IsAsciiCharacter( Codepoint );

  struct CodepointView
  {
    CodepointView() = default;
    CodepointView( const Codepoint*, int );
    Codepoint        operator[]( int ) const;
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

  CodepointView LocalizationGetString( Language, StringView );
  void          LocalizationLoad( const Filesystem::Path&, Errors& );
  void          LocalizationDebugImgui();
} // namespace Tac
