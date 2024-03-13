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
    LocalizedStringStuff( CodepointView cv, StringView sv )
    {
      SetCodepointsOnly( cv );
      SetUTF8StringOnly( sv );
    }

    void                SetCodepoints( CodepointView );

    void                SetCodepointsAndUTF8String( CodepointView cv, StringView sv )
    {
      SetCodepointsOnly( cv );
      SetUTF8StringOnly( sv );
    }

    void                SetUTF8String( StringView );

    StringView          GetUTF8String() const
    {
      return mUTF8String;
    }

    CodepointView       GetCodepointView() const
    {
      return CodepointView( mCodepoints.data(),
                            mCodepoints.size() );
    }

  private:
    void SetUTF8StringOnly( StringView );
    void SetCodepointsOnly( CodepointView );

    // TODO: don't bother storing the codepoints, just compute them on the fly
    Vector< Codepoint > mCodepoints;
    String              mUTF8String;

  };

  typedef Map< Language, LocalizedStringStuff > LanguageMap;

  struct LocalizedString
  {
    String               mReference;
    LanguageMap          mCodepoints;
  };


  static void             LoadLanguageMapEntry( LanguageMap* languageMap, ParseData& parseData )
  {
    const StringView word = parseData.EatWord();
    const Language language = GetLanguage( word );
    TAC_ASSERT( language != Language::Count );
    parseData.EatWhitespace();
    const StringView utf8String = parseData.EatRestOfLine();
    const CodepointView codepointView = UTF8ToCodepoints( utf8String );
    TAC_ASSERT( !codepointView.empty() );

    LocalizedStringStuff localizedStringStuff( codepointView, utf8String );

    ( *languageMap )[ language ] = localizedStringStuff;
  }

  static bool             LoadLocalizedString( LocalizedString* localizedString, ParseData& parseData )
  {
    const StringView reference = parseData.EatRestOfLine();
    if( reference.empty() )
      return false;

    LanguageMap codepointMap;
    while( !parseData.EatWhitespace() && parseData.GetRemainingByteCount())
      LoadLanguageMapEntry( &codepointMap, parseData );

    //localizedString->mReference = reference;
    //localizedString->mCodepoints = codepointMap;
    *localizedString = LocalizedString
    {
      .mReference = reference,
      .mCodepoints = codepointMap,
    };
    return true;
  }

  static Vector< LocalizedString > mLocalizedStrings;

  static LocalizedString* FindLocalizedString( StringView reference )
  {
    for( LocalizedString& str : mLocalizedStrings )
      if( ( StringView )str.mReference == reference )
        return &str;
    return nullptr;
  }


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

  StringView LanguageToStr( Language language )
  {
    return Languages[ ( int )language ];
  }

  Language GetLanguage( StringView str )
  {
    for( int i = 0; i < ( int )Language::Count; ++i )
      if( ( StringView )Languages[ i ] == str )
        return ( Language )i;
    return Language::Count;
  }

  bool IsAsciiCharacter( Codepoint codepoint )
  {
    return codepoint < ( Codepoint )128;
  }

  void LocalizedStringStuff::SetUTF8String( StringView utf8String )
  {
    SetUTF8StringOnly( utf8String );
    SetCodepointsOnly( UTF8ToCodepoints( utf8String ) );
  }

  void LocalizedStringStuff::SetUTF8StringOnly( StringView utf8String)
  {
    mUTF8String = utf8String;
  }
      
  void LocalizedStringStuff::SetCodepointsOnly( CodepointView codepoints )
  {
    mCodepoints.resize( codepoints.size() );
    for( int i = 0; i < codepoints.size(); ++i )
      mCodepoints[ i ] = codepoints[ i ];
  }

  void LocalizedStringStuff::SetCodepoints( CodepointView codepoints )
  {
    SetCodepointsOnly( codepoints );
    SetUTF8StringOnly( CodepointsToUTF8( codepoints ) );
  }

  //===--- Converter ---===//

  struct Converter
  {
    Converter( StringView stringView );
    Codepoint   Extract();
  private:
    char        GetNextByte();
    const char* mBegin = nullptr;
    const char* mEnd = nullptr;
  };

  Converter::Converter( StringView stringView )
  {
    mBegin = stringView.begin();
    mEnd = stringView.end();
  }

  Codepoint Converter::Extract()
  {
    const char b0 = GetNextByte();
    if( !b0 )
      return 0;
    if( ( b0 & 0b10000000 ) == 0 )
    {
      return ( 0b01111111 & b0 ) << 0;
    }

    const char b1 = GetNextByte();
    if( !b1 )
      return 0;
    if( ( b0 & 0b00100000 ) == 0 )
    {
      return
        ( ( 0b00111111 & b1 ) << 0 ) |
        ( ( 0b00011111 & b0 ) << 6 );
    }

    const char b2 = GetNextByte();
    if( !b2 )
      return 0;
    if( ( b0 & 0b00010000 ) == 0 )
    {
      return
        ( ( 0b00111111 & b2 ) << 0 ) |
        ( ( 0b00111111 & b1 ) << 6 ) |
        ( ( 0b00001111 & b0 ) << 12 );
    }

    const char b3 = GetNextByte();
    if( !b3 )
      return 0;
    return
      ( ( 0b00111111 & b3 ) << 0 ) |
      ( ( 0b00111111 & b2 ) << 6 ) |
      ( ( 0b00111111 & b1 ) << 12 ) |
      ( ( 0b00000111 & b0 ) << 18 );
  }

  char      Converter::GetNextByte()
  {
    return mBegin < mEnd ? *mBegin++ : 0;
  }


  //===--- Codepoint View ---===//

  CodepointView::CodepointView( const Codepoint* codepoints, int codepointCount )
  {
    mCodepoints = codepoints;
    mCodepointCount = codepointCount;
  }

  const Codepoint* CodepointView::data() const
  {
    return mCodepoints;

  }

  const Codepoint* CodepointView::begin() const
  {
    return mCodepoints;
  }

  const Codepoint* CodepointView::end() const
  {
    return mCodepoints + mCodepointCount;
  }

  int              CodepointView::size() const
  {
    return mCodepointCount;
  }

  bool             CodepointView::empty() const
  {
    return mCodepointCount == 0;
  }

  Codepoint        CodepointView::operator[]( int i ) const
  {
    return mCodepoints[ i ];
  }

  bool operator != ( CodepointView a, CodepointView b )
  {
    return !( a == b );
  }

  bool operator == ( CodepointView a, CodepointView b )
  {
    if( a.size() != b.size() )
      return false;
    for( int i = 0; i < a.size(); ++i )
      if( a[ i ] != b[ i ] )
        return false;
    return true;
  }


  CodepointView UTF8ToCodepoints( StringView stringView )
  {
    auto codepoints = ( Codepoint* )FrameMemoryAllocate( stringView.size() * sizeof( Codepoint ) );
    int n = 0;
    Converter converter( stringView );
    while( Codepoint codepoint = converter.Extract() )
      codepoints[ n++ ] = codepoint;
    return CodepointView( codepoints, n );
  }

  StringView CodepointsToUTF8( CodepointView codepointView )
  {
    auto str = ( char* )FrameMemoryAllocate( codepointView.size() * sizeof( Codepoint ) );
    int len = 0;

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

  //===--- Localization ---===//

  CodepointView LocalizationGetString( Language language, StringView reference )
  {
    LocalizedString* str = FindLocalizedString( reference );
    if( !str )
      return {};


    auto it = str->mCodepoints.Find(language);
    TAC_ASSERT( it );

    const LocalizedStringStuff& localizedStringStuff = (*it).mSecond;
    return localizedStringStuff.GetCodepointView();
  }

  void          LocalizationLoad( const Filesystem::Path& path, Errors& errors )
  {
    TAC_CALL( const String str = LoadFilePath( path, errors ) );

    ParseData parseData( str.data(), str.size() );

    LocalizedString localizedString;
    while( LoadLocalizedString( &localizedString, parseData ) )
      mLocalizedStrings.push_back( localizedString );
  }

  void          LocalizationDebugImgui()
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
  void LanguageDebugImgui( StringView , Language* )
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
