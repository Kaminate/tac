#include "src/common/tacLocalization.h"
#include "src/common/tacUtility.h"
#include "src/common/tacMemory.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacTemporaryMemory.h"

namespace Tac
{

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
      if( Languages[ i ] == str )
        return ( Language )i;
    return Language::Count;
  }

  bool IsAsciiCharacter( Codepoint codepoint )
  {
    return codepoint < ( Codepoint )128;
  }

  void LocalizedStringStuff::SetCodepoints( CodepointView codepoints )
  {
    mCodepoints.resize( codepoints.size() );
    for( int i = 0; i < codepoints.size(); ++i )
      mCodepoints[ i ] = codepoints[ i ];
  }

  //struct UTF8Converter
  //{
  //  static void Convert( StringView text,
  //                       Vector< Codepoint >& codepoints,
  //                       Errors& errors );
  //  static void Convert( const Vector< Codepoint >& codepoints,
  //                       String& text );
  //  void Run( Vector< Codepoint >& codepoints, Errors& errors );
  //  void IterateUTF8( Codepoint* codepoint, Errors& errors );
  //  char GetNextByte( Errors& errors );
  //  const char* mBegin = nullptr;
  //  const char* mEnd = nullptr;
  //};


  //char UTF8Converter::GetNextByte( Errors& errors )
  //{
  //  if( mBegin >= mEnd )
  //  {
  //    errors = "utf8 get next byte failed";
  //    return 0;
  //  }
  //  return *mBegin++;
  //}
  //void UTF8Converter::IterateUTF8( Codepoint* codepoint, Errors& errors )
  //{
  //}
  //void UTF8Converter::Run( Vector< Codepoint >& codepoints, Errors& errors )
  //{
  //  while( mBegin < mEnd )
  //  {
  //    Codepoint codepoint;
  //    IterateUTF8( &codepoint, errors );
  //    TAC_HANDLE_ERROR( errors );
  //    codepoints.push_back( codepoint );
  //  }
  //}
  //void UTF8Converter::Convert( StringView text, Vector< Codepoint >& codepoints )
  //{
  //  UTF8Converter converter;
  //  converter.mBegin = text.data();
  //  converter.mEnd = text.data() + text.size();
  //  converter.Run( codepoints, errors );
  //}
  //void UTF8Converter::Convert( const Vector< Codepoint >& codepoints,
  //                             String& text )
  //{
  //  for( Codepoint codepoint : codepoints )
  //  {
  //    if( codepoint >= 0x10000 )
  //    {
  //      text.push_back( 0b11110000 | ( 0b00000111 & ( codepoint >> 18 ) ) );
  //      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 12 ) ) );
  //      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 6 ) ) );
  //      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 0 ) ) );
  //    }
  //    else if( codepoint >= 0x800 )
  //    {
  //      text.push_back( 0b11100000 | ( 0b00001111 & ( codepoint >> 12 ) ) );
  //      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 6 ) ) );
  //      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 0 ) ) );
  //    }
  //    else if( codepoint > 0x80 )
  //    {
  //      text.push_back( 0b11000000 | ( 0b00011111 & ( codepoint >> 6 ) ) );
  //      text.push_back( 0b10000000 | ( 0b00111111 & ( codepoint >> 0 ) ) );
  //    }
  //    else
  //    {
  //      text.push_back( 0b01111111 & codepoint );
  //    }
  //  }
  //}



  struct Converter
  {
    Converter( StringView stringView );
    Codepoint Extract();
  private:
    char GetNextByte();
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

  char Converter::GetNextByte()
  {
    return mBegin < mEnd ? *mBegin++ : 0;
  }



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

  int CodepointView::size() const
  {
    return mCodepointCount;
  }

  bool CodepointView::empty() const
  {
    return mCodepointCount == 0;
  }
  
  Codepoint CodepointView::operator[]( int i ) const
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
    auto codepoints = ( Codepoint* )FrameMemory::Allocate( stringView.size() * sizeof( Codepoint ) );
    int n = 0;
    Converter converter( stringView );
    while( Codepoint codepoint = converter.Extract() )
      codepoints[ n++ ] = codepoint;
    return CodepointView( codepoints, n );
  }

  StringView CodepointsToUTF8( CodepointView codepointView )
  {
    auto str = ( char* )FrameMemory::Allocate( codepointView.size() * sizeof( Codepoint ) );
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


  String Localization::EatWord()
  {
    String result;
    while( mBegin < mEnd )
    {
      if( ( EatWhitespace() || EatNewLine() ) && !result.empty() )
        break;
      result += *mBegin++;
    }
    return result;
  }

  const Vector< Codepoint >& Localization::GetString( Language language, StringView reference )
  {
    for( const auto& localizedString : mLocalizedStrings )
    {
      if( localizedString.mReference != reference )
        continue;
      return localizedString.mCodepoints.at( language ).mCodepoints;
    }
    TAC_INVALID_CODE_PATH;
    static Vector< Codepoint > result;
    return result;
  }

  Localization* Localization::Instance = nullptr;
  
  Localization::Localization()
  {
    Instance = this;
  }

  void Localization::Load( StringView path, Errors& errors )
  {
    mBytes = TemporaryMemoryFromFile( path, errors );
    TAC_HANDLE_ERROR( errors );
    mBegin = mBytes.data();
    mEnd = mBytes.data() + mBytes.size();

    String reference;
    while( mBegin < mEnd )
    {
      if( reference.empty() )
        reference = EatWord();
      LocalizedString localizedString;
      localizedString.mReference = reference;
      while( mBegin < mEnd )
      {
        auto word = EatWord();
        auto language = GetLanguage( word );
        if( language == Language::Count )
        {
          reference = word;
          break;
        }
        EatWhitespace();

        const char* stringBegin = mBegin;
        while( mBegin < mEnd )
        {
          if( EatNewLine() )
            break;
          mBegin++;
        }

        StringView utf8String( stringBegin, ( int )( mBegin - stringBegin ) );
        CodepointView codepoints = UTF8ToCodepoints( utf8String );

        if( codepoints.empty() )
        {
          errors += "Failed reading " + reference + " of " + word;
          return;
        }
        LocalizedStringStuff localizedStringStuff;
        localizedStringStuff.SetCodepoints( codepoints );
        localizedStringStuff.mUTF8String = utf8String;
        localizedString.mCodepoints[ language ] = localizedStringStuff;
      }
      mLocalizedStrings.push_back( localizedString );
    }
    mBytes.clear();
    mBegin = nullptr;
    mEnd = nullptr;
  }

  bool Localization::EatNewLine()
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

  bool Localization::EatWhitespace()
  {
    auto oldBegin = mBegin;
    while( mBegin < mEnd )
    {
      if( !Contains( { ' ', '\t' }, *mBegin ) )
        break;
      mBegin++;
    }
    return oldBegin < mBegin;
  }

  void Localization::DebugImgui()
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
  void LanguageDebugImgui( StringView name, Language* language )
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
