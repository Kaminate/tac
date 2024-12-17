// This file deals with localizing to different languages
//
// The way localization works is that a string reference is
// used to refer to phrase in any given language.
//
// The codepoints in a phrase are encoded into a utf-8 string

#pragma once

//#include "tac-std-lib/tac_core.h"

#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac{ struct StringView; struct Errors; }
namespace Tac::FileSys { struct Path; }

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

  using CodepointString = Vector< Codepoint >;

  // shouldnt this just be using codepointview = Span< const Codepoint> ?
  struct CodepointView
  {
    CodepointView() = default;
    CodepointView( const Codepoint*, int );
    CodepointView( const CodepointString& );
    Codepoint        operator[]( int ) const;
    const Codepoint* data() const;
    const Codepoint* begin() const;
    const Codepoint* end() const;
    int              size() const;
    bool             empty() const;
    const Codepoint* mCodepoints     {};
    int              mCodepointCount {};
  };


  bool operator == ( CodepointView, CodepointView );
  bool operator != ( CodepointView, CodepointView );

  StringView      LanguageToStr( Language );
  Language        GetLanguage( StringView );
  void            LanguageDebugImgui( StringView name, Language* );
  bool            IsAsciiCharacter( Codepoint );

  CodepointString UTF8ToCodepointString( StringView );
  CodepointView   UTF8ToCodepointView( StringView );
  StringView      CodepointsToUTF8( CodepointView );

  CodepointView   LocalizationGetString( Language, StringView );
  void            LocalizationLoad( const FileSys::Path&, Errors& );
  void            LocalizationDebugImgui();
} // namespace Tac
