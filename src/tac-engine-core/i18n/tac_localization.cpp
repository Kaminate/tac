#include "tac_localization.h" // self-inc

#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"


namespace Tac
{
  struct LocalizedStringStuff
  {
    LocalizedStringStuff() = default;
    LocalizedStringStuff( CodepointString, StringView );
    LocalizedStringStuff( CodepointView, StringView );
    void SetCodepoints( CodepointView );
    void SetCodepointsAndUTF8String( CodepointView , StringView );
    void SetUTF8String( StringView );
    auto GetUTF8String() const -> StringView;
    auto GetCodepointView() const -> CodepointView;

  private:
    void SetUTF8StringOnly( StringView );
    void SetCodepointsOnly( CodepointView );
    void SetCodepointsOnly( CodepointString );

    // TODO: don't bother storing the codepoints, just compute them on the fly
    Vector< Codepoint > mCodepoints;
    String              mUTF8String;

  };

  using LanguageMap = Map< Language, LocalizedStringStuff >;

  struct LocalizedString
  {
    String      mReference;
    LanguageMap mCodepoints;
  };

  using LocalizedStrings = Vector< LocalizedString >;
  using Languages = String[ ( int )Language::Count ];

  static LocalizedStrings sLocalizedStrings;
  static const Languages  kLanguages
  {
    "Arabic",
    "Chinese",
    "English",
    "Japanese",
    "Korean",
    "Russian",
    "Spanish",
  };

  static void LoadLanguageMapEntry( LanguageMap* languageMap, ParseData& parseData )
  {
    const StringView word{ parseData.EatWord() };
    const Language language{ GetLanguage( word ) };
    TAC_ASSERT( language != Language::Count );
    parseData.EatWhitespace();
    const StringView utf8String{ parseData.EatRestOfLine() };
    const CodepointString codepointString{ UTF8ToCodepointString( utf8String ) };
    const CodepointView codepointView{ codepointString.data(), codepointString.size() };
    TAC_ASSERT( !codepointString.empty() );

    LocalizedStringStuff& localizedStringStuff = ( *languageMap )[ language ];
    localizedStringStuff.SetCodepointsAndUTF8String( codepointView, utf8String );
  }

  static bool LoadLocalizedString( LocalizedString* localizedString, ParseData& parseData )
  {
    const StringView reference { parseData.EatRestOfLine() };
    if( reference.empty() )
      return false;

    LanguageMap codepointMap;
    while( !parseData.EatWhitespace() && parseData.GetRemainingByteCount() )
      LoadLanguageMapEntry( &codepointMap, parseData );

    //localizedString->mReference = reference;
    //localizedString->mCodepoints = codepointMap;
    *localizedString = LocalizedString
    {
      .mReference { reference },
      .mCodepoints { codepointMap },
    };
    return true;
  }

  static auto FindLocalizedString( StringView reference ) -> LocalizedString*
  {
    for( LocalizedString& str : sLocalizedStrings )
      if( ( StringView )str.mReference == reference )
        return &str;
    return nullptr;
  }


  LocalizedStringStuff::LocalizedStringStuff( CodepointString cv, StringView sv )
  {
    SetCodepointsOnly( cv );
    SetUTF8StringOnly( sv );
  }

  LocalizedStringStuff::LocalizedStringStuff( CodepointView cv, StringView sv )
  {
    SetCodepointsOnly( cv );
    SetUTF8StringOnly( sv );
  }

  void LocalizedStringStuff::SetUTF8String( StringView utf8String )
  {
    SetUTF8StringOnly( utf8String );
    SetCodepointsOnly( UTF8ToCodepointString( utf8String ) );
  }

  auto LocalizedStringStuff::GetUTF8String() const -> StringView { return mUTF8String; }

  auto LocalizedStringStuff::GetCodepointView() const -> CodepointView
  {
    return CodepointView( mCodepoints.data(), mCodepoints.size() );
  }

  void LocalizedStringStuff::SetUTF8StringOnly( StringView utf8String )
  {
    mUTF8String = utf8String;
  }

  void LocalizedStringStuff::SetCodepointsOnly( CodepointString codepoints )
  {
    CodepointView view( codepoints.data(), codepoints.size() );
    SetCodepointsOnly( view );
  }

  void LocalizedStringStuff::SetCodepointsOnly( CodepointView codepoints )
  {
    mCodepoints.resize( codepoints.size() );
    for( int i{}; i < codepoints.size(); ++i )
      mCodepoints[ i ] = codepoints[ i ];
  }

  void LocalizedStringStuff::SetCodepoints( CodepointView codepoints )
  {
    SetCodepointsOnly( codepoints );
    SetUTF8StringOnly( CodepointsToUTF8( codepoints ) );
  }

  void LocalizedStringStuff::SetCodepointsAndUTF8String( CodepointView cv, StringView sv )
  {
    SetCodepointsOnly( cv );
    SetUTF8StringOnly( sv );
  }

  //===--- Converter ---===//

  struct Converter
  {
    Converter( StringView stringView );
    Codepoint   Extract();
  private:
    char        GetNextByte();

    const char* mBegin {};
    const char* mEnd   {};
  };

  Converter::Converter( StringView stringView )
  {
    mBegin = stringView.begin();
    mEnd = stringView.end();
  }

  auto Converter::Extract() -> Codepoint
  {
    const char b0 { GetNextByte() };
    if( !b0 )
      return 0;

    if( ( b0 & 0b10000000 ) == 0 )
      return ( 0b01111111 & b0 ) << 0;

    const char b1 { GetNextByte() };
    if( !b1 )
      return 0;

    if( ( b0 & 0b00100000 ) == 0 )
      return
        ( ( 0b00111111 & b1 ) << 0 ) |
        ( ( 0b00011111 & b0 ) << 6 );

    const char b2 { GetNextByte() };
    if( !b2 )
      return 0;

    if( ( b0 & 0b00010000 ) == 0 )
      return
        ( ( 0b00111111 & b2 ) << 0 ) |
        ( ( 0b00111111 & b1 ) << 6 ) |
        ( ( 0b00001111 & b0 ) << 12 );

    const char b3 { GetNextByte() };
    if( !b3 )
      return 0;

    return
      ( ( 0b00111111 & b3 ) << 0 ) |
      ( ( 0b00111111 & b2 ) << 6 ) |
      ( ( 0b00111111 & b1 ) << 12 ) |
      ( ( 0b00000111 & b0 ) << 18 );
  }

  char Converter::GetNextByte()
  {
    return mBegin < mEnd ? *mBegin++ : 0;
  }

  //===--- Codepoint View ---===//

  CodepointView::CodepointView( const Codepoint* codepoints, int codepointCount )
  {
    mCodepoints = codepoints;
    mCodepointCount = codepointCount;
  }

  CodepointView::CodepointView( const CodepointString& codepointString )
  {
    mCodepoints = codepointString.data();
    mCodepointCount = codepointString.size();
  }

  auto CodepointView::data() const -> const Codepoint*
  {
    return mCodepoints;

  }

  auto CodepointView::begin() const -> const Codepoint*
  {
    return mCodepoints;
  }

  auto CodepointView::end() const -> const Codepoint*
  {
    return mCodepoints + mCodepointCount;
  }

  auto CodepointView::size() const -> int
  {
    return mCodepointCount;
  }

  bool CodepointView::empty() const
  {
    return mCodepointCount == 0;
  }

  auto CodepointView::operator[]( int i ) const -> Codepoint
  {
    return mCodepoints[ i ];
  }
}

bool Tac::operator != ( CodepointView a, CodepointView b )
{
  return !( a == b );
}

bool Tac::operator == ( CodepointView a, CodepointView b )
{
  if( a.size() != b.size() )
    return false;
  for( int i{}; i < a.size(); ++i )
    if( a[ i ] != b[ i ] )
      return false;
  return true;
}

auto Tac::LanguageToStr( Language language ) -> StringView
{
  return kLanguages[ ( int )language ];
}

auto Tac::GetLanguage( StringView str ) -> Language
  {
    for( int i{}; i < ( int )Language::Count; ++i )
      if( ( StringView )kLanguages[ i ] == str )
        return ( Language )i;
    return Language::Count;
  }

auto Tac::UTF8ToCodepointString( StringView stringView ) -> CodepointString
{
  CodepointString codepoints;
  Converter converter( stringView );
  while( Codepoint codepoint { converter.Extract() } )
    codepoints.push_back( codepoint );
  return codepoints;
}

auto Tac::UTF8ToCodepointView( StringView stringView ) -> CodepointView
{
  Codepoint* codepoints{
    ( Codepoint* )FrameMemoryAllocate( sizeof( Codepoint ) * stringView.size() ) };
  int n{};
  Converter converter( stringView );
  while( Codepoint codepoint{ converter.Extract() } )
    codepoints[ n++ ] = codepoint;

  return CodepointView( codepoints, n );
}

auto Tac::CodepointsToUTF8( CodepointView codepointView ) -> StringView
{
  auto str { ( char* )FrameMemoryAllocate( codepointView.size() * sizeof( Codepoint ) ) };
  int len {};

  for( Codepoint codepoint : codepointView )
  {
    if( codepoint >= 0x10000 )
    {
      str[ len++ ] = 0b11110000 | ( 0b00000111 & ( codepoint >> 18 ) );
      str[ len++ ] = 0b10000000 | ( 0b00111111 & ( codepoint >> 12 ) );
      str[ len++ ] = 0b10000000 | ( 0b00111111 & ( codepoint >> 6 ) );
      str[ len++ ] = 0b10000000 | ( 0b00111111 & ( codepoint >> 0 ) );
    }
    else if( codepoint >= 0x800 )
    {
      str[ len++ ] = 0b11100000 | ( 0b00001111 & ( codepoint >> 12 ) );
      str[ len++ ] = 0b10000000 | ( 0b00111111 & ( codepoint >> 6 ) );
      str[ len++ ] = 0b10000000 | ( 0b00111111 & ( codepoint >> 0 ) );
    }
    else if( codepoint > 0x80 )
    {
      str[ len++ ] = 0b11000000 | ( 0b00011111 & ( codepoint >> 6 ) );
      str[ len++ ] = 0b10000000 | ( 0b00111111 & ( codepoint >> 0 ) );
    }
    else
    {
      str[ len++ ] = 0b01111111 & codepoint;
    }

  }
  return StringView( str, len );
}

auto Tac::LocalizationGetString( Language language, StringView reference ) -> CodepointView
{
  LocalizedString* str{ FindLocalizedString( reference ) };
  if( !str )
    return {};

  auto it{ str->mCodepoints.Find( language ) };
  TAC_ASSERT( it != str->mCodepoints.end() );

  auto& [_, localizedStringStuff] {*it};
  return localizedStringStuff.GetCodepointView();
}

void Tac::LocalizationLoad( const UTF8Path& path, Errors& errors )
{
  TAC_CALL( const String str{ path.LoadFilePath( errors ) }  );
  ParseData parseData( str.data(), str.size() );
  LocalizedString localizedString;
  while( LoadLocalizedString( &localizedString, parseData ) )
    sLocalizedStrings.push_back( localizedString );
}

void Tac::LocalizationDebugImgui()
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

void Tac::LanguageDebugImgui( StringView, Language* )
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

bool Tac::IsAsciiCharacter( Codepoint codepoint ) { return codepoint < ( Codepoint )128; }

