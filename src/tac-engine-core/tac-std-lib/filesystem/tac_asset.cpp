#include "tac-std-lib/filesystem/tac_asset.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h" // IsDebugMode
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/error/tac_assert.h"
#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h" 
#endif
//#include "tac-std-lib/shell/tac_shell.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  static bool IsValid( const char c )
  {
    return IsAlpha( c )
      || IsDigit( c )
      || c == '.'
      || c == '_'
      || c == ' '
      || c == AssetPathSeperator;
  }

  static bool IsValid( const StringView& s )
  {
    if( s.empty() )
      return true;

    if( !s.starts_with( AssetPathRootFolderName ) )
      return false;

    for( char c : s )
      if( !IsValid( c ) )
        return false;

    if( s.back() == AssetPathSeperator )
      return false;

    return true;
  }

  static void Validate( const StringView& s )
  {
    if( !IsDebugMode || IsValid( s ) )
      return;

    const char quote = '\"';

    String msg = "Invalid Asset path ";
    msg += quote;
    msg += s;
    msg += quote;

    TAC_ASSERT_CRITICAL( msg );
  }

  static Filesystem::IterateType AssetToFSIterateType( AssetIterateType t )
  {
    switch( t )
    {
    case AssetIterateType::Default: return Filesystem::IterateType::Default;
    case AssetIterateType::Recursive: return Filesystem::IterateType::Recursive;
    default: TAC_ASSERT_INVALID_CASE( t ); return Filesystem::IterateType::Default;
    }
  }

  // -----------------------------------------------------------------------------------------------

  // AssetPathStringView

  AssetPathStringView::AssetPathStringView( const char* s ) : StringView( s )
  {
    Validate( s );
  }

  AssetPathStringView::AssetPathStringView( const String& s ) : StringView( s )
  {
    Validate( s );
  }

  AssetPathStringView::AssetPathStringView( const AssetPathString& s ) : StringView( s )
  {
    // already validated
  }

  AssetPathStringView::AssetPathStringView( const StringView& s ) : StringView( s )
  {
    Validate( s );
  };

  AssetPathStringView AssetPathStringView::GetDirectory() const
  {
    TAC_ASSERT( IsFile() );
    return substr( 0, find_last_of( AssetPathSeperator ) );
  }

  StringView          AssetPathStringView::GetFilename() const
  {
    TAC_ASSERT( IsFile() );
    return substr( find_last_of( AssetPathSeperator ) + 1 );
  }

  StringView          AssetPathStringView::GetFileExtension() const
  {
    TAC_ASSERT( IsFile() );
    return substr( find_last_of( '.' ) ); // returned string includes the '.', ie ".png"
  }

  bool                AssetPathStringView::IsFile() const
  {
    if( empty() )
      return false;

    const int lastSlash = find_last_of( AssetPathSeperator );
    const int lastDot = find_last_of( '.' );
    TAC_ASSERT( lastSlash != StringView::npos );
    TAC_ASSERT( lastDot != StringView::npos );
    return lastDot > lastSlash;
  }

  bool                AssetPathStringView::IsDirectory() const
  {
    return !empty() && !IsFile();
  }

  // -----------------------------------------------------------------------------------------------

  // AssetPathString

  AssetPathString::AssetPathString( const AssetPathStringView& s ) : String( s ) {}

  void AssetPathString::operator = ( const String& s )
  {
    assign( s );
    Validate( s );
  }
}

  // -----------------------------------------------------------------------------------------------

  bool                  Tac::Exists( const AssetPathStringView& assetPath )
  {
    const Filesystem::Path fsPath( assetPath );
    return Exists( fsPath );
  }

  void                  Tac::SaveToFile( const AssetPathStringView& assetPath,
                                         const void* bytes,
                                         int byteCount,
                                         Errors& errors)
  {
    const Filesystem::Path fsPath( assetPath );
    Filesystem::SaveToFile( fsPath, bytes, byteCount, errors );
  }

  Tac::String           Tac::LoadAssetPath( const AssetPathStringView& assetPath,
                                            Errors& errors )
  {
    const Filesystem::Path fsPath( assetPath );
    return Filesystem::LoadFilePath( fsPath, errors );
  }

  Tac::AssetPathStrings Tac::IterateAssetsInDir( const AssetPathStringView& dir,
                                               AssetIterateType type,
                                               Errors& errors )
  {
    const Filesystem::IterateType fsIterate = AssetToFSIterateType( type );

    const Filesystem::Paths paths =
      TAC_CALL_RET( {}, Filesystem::IterateFiles( dir, fsIterate, errors ) );

    AssetPathStrings result;
    for( const Filesystem::Path& path : paths )
    {
      OS::OSDebugBreak();
      String s = path.u8string();
      s.replace("\\", "/");
      int i = s.find( "assets/" );
      s = s.substr(i);

    AssetPathString assetPath;

      //const AssetPathStringView assetPath = ModifyPathRelative( path, errors );
      result.push_back( assetPath );
    }

    return result;
  }
  // -----------------------------------------------------------------------------------------------
