// This file deals with localizing to different languages
//
// The way localization works is that a string reference is
// used to refer to phrase in any given language.
//
// The codepoints in a phrase are encoded into a utf-8 string

#pragma once

#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/string/tac_string_view.h"

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
    auto operator[]( int ) const -> Codepoint;
    auto data() const -> const Codepoint*;
    auto begin() const -> const Codepoint*;
    auto end() const -> const Codepoint*;
    auto size() const -> int;
    auto empty() const -> bool;
    const Codepoint* mCodepoints     {};
    int              mCodepointCount {};
  };

  bool operator == ( CodepointView, CodepointView );
  bool operator != ( CodepointView, CodepointView );

  auto LanguageToStr( Language ) -> StringView;
  auto GetLanguage( StringView ) -> Language;
  auto LanguageDebugImgui( StringView name, Language* ) -> void;
  auto IsAsciiCharacter( Codepoint ) -> bool;

  auto UTF8ToCodepointString( StringView ) -> CodepointString;
  auto UTF8ToCodepointView( StringView ) -> CodepointView;
  auto CodepointsToUTF8( CodepointView ) -> StringView;

  auto LocalizationGetString( Language, StringView ) -> CodepointView;
  auto LocalizationLoad( const UTF8Path&, Errors& ) -> void;
  auto LocalizationDebugImgui() -> void;
} // namespace Tac
