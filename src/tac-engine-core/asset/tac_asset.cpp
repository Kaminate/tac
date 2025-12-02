#include "tac_asset.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h" // IsDebugMode
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/meta/tac_meta_impl.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"

namespace Tac
{

  struct MetaAssetPathString : public MetaType
                                                                                                    {
    const char* GetName() const override                                                            { return "AssetPathString"; }
    int         GetSizeOf() const override                                                          { return sizeof( AssetPathString ); }
    String      ToString( const void* ) const override                                              { TAC_ASSERT_UNIMPLEMENTED; return {}; }
    float       ToNumber( const void* ) const override                                              { TAC_ASSERT_UNIMPLEMENTED; return {}; }
    void        Cast( CastParams ) const  override                                                  { TAC_ASSERT_UNIMPLEMENTED; }

    void        JsonSerialize( Json* json, const void* v ) const override                           { json->SetString( ToRef( v ) ); }
    void        JsonDeserialize( const Json* json, void* v ) const override                         { ToRef( v ) = json->mString; }

    void        Read( ReadStream* , dynmc void* ) const override                                    { TAC_ASSERT_UNIMPLEMENTED; }
    void        Write( WriteStream* , const void* ) const override                                  { TAC_ASSERT_UNIMPLEMENTED; }

    bool        Equals( const void* a, const void* b ) const override                               { return ToRef( a ) == ToRef( b ); }
    void        Copy( CopyParams copyParams ) const override                                        { ToRef( copyParams.mDst ) = ToRef( copyParams.mSrc ); }

  private:
    dynmc AssetPathString& ToRef( dynmc void* v ) const                                             { return *( AssetPathString* )v; }
    const AssetPathString& ToRef( const void* v ) const                                             { return *( AssetPathString* )v; }
  };

  static const UTF8Path sInitialWorkingDir   { UTF8Path::GetCurrentWorkingDirectory() };
  static const char          sAssetPathSeperator      { '/' };
  static const char*         sAssetPathRootFolderName { "assets" };
  TAC_META_IMPL_INSTANCE2( AssetPathString, MetaAssetPathString );

  // -----------------------------------------------------------------------------------------------

#if TAC_IS_DEBUG_MODE()
  static bool IsValid( const char c )
  {
    return IsAlpha( c )
      || IsDigit( c )
      || c == '.'
      || c == '_'
      || c == '-'
      || c == ' '
      || c == sAssetPathSeperator;
  }

  static bool IsValid( const StringView s )
  {
    if( s.empty() )
      return true;

    if( !s.starts_with( sAssetPathRootFolderName ) )
      return false;

    for( char c : s )
      if( !IsValid( c ) )
        return false;

    if( s.back() == sAssetPathSeperator )
      return false;

    return true;
  }
#endif

  static void Validate( const StringView s )
  {
#if TAC_IS_DEBUG_MODE()
      if( !IsValid( s ) )
      {
        const char quote{ '\"' };
        String msg{ String() + "Invalid Asset path " + quote + s + quote };
        TAC_ASSERT_CRITICAL( msg );
      }
#else
    TAC_UNUSED_PARAMETER( s );
#endif
  }

  static UTF8Path::IterateType AssetToFSIterateType( AssetIterateType t )
  {
    switch( t )
    {
    case AssetIterateType::Default: return UTF8Path::IterateType::Default;
    case AssetIterateType::Recursive: return UTF8Path::IterateType::Recursive;
    default: TAC_ASSERT_INVALID_CASE( t ); return UTF8Path::IterateType::Default;
    }
  }

  // -----------------------------------------------------------------------------------------------

  // AssetPathStringView

  AssetPathStringView::AssetPathStringView( const char* s ) : StringView( s )                       { Validate( s ); }
  AssetPathStringView::AssetPathStringView( const String& s ) : StringView( s )                     { Validate( s ); }
  AssetPathStringView::AssetPathStringView( const AssetPathString& s ) : StringView( s )            {} // already validated
  AssetPathStringView::AssetPathStringView( const StringView s ) : StringView( s )                 { Validate( s ); };

  auto AssetPathStringView::GetDirectory() const -> AssetPathStringView
  {
    TAC_ASSERT( IsFile() );
    return substr( 0, find_last_of( sAssetPathSeperator ) );
  }

  auto AssetPathStringView::GetFilename() const -> StringView
  {
    return substr( find_last_of( sAssetPathSeperator ) + 1 );
  }

  auto AssetPathStringView::GetFileExtension() const -> StringView
  {
    TAC_ASSERT( IsFile() );
    return substr( find_last_of( '.' ) ); // returned string includes the '.', ie ".png"
  }

  bool AssetPathStringView::IsFile() const
  {
    if( empty() )
      return false;

    const int lastSlash { find_last_of( sAssetPathSeperator ) };
    const int lastDot { find_last_of( '.' ) };
    TAC_ASSERT( lastSlash != StringView::npos );
    TAC_ASSERT( lastDot != StringView::npos );
    return lastDot > lastSlash;
  }

  bool AssetPathStringView::IsDirectory() const
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

  static auto StripLeadingSlashes( const StringView path ) -> String
  {
    int i{};
    for( i; i < path.size(); i++ )
      if( path[ i ] != '/' && path[ i ] != '\\' )
        break;
    return path.substr( i );
  }

  static auto ModifyPathRelative( const UTF8Path& path, Errors& errors ) -> Tac::AssetPathStringView
  {
    const String workingUTF8 { sInitialWorkingDir };
    dynmc String UTF8Path { path };
    if( path.is_absolute() )
    {
      TAC_RAISE_ERROR_IF_RETURN( !UTF8Path.starts_with( workingUTF8 ), 
                                 String() + UTF8Path + String( " is not in " ) + workingUTF8 );
      UTF8Path.erase( 0, workingUTF8.size() );
      UTF8Path = StripLeadingSlashes( UTF8Path );
    }

    UTF8Path.replace( "\\", "/");
    return FrameMemoryCopy( UTF8Path.c_str() );
  }

  static AssetPathStrings ConvertAssetPaths( const UTF8Paths& paths )
  {
    AssetPathStrings result;
    for( const UTF8Path& path : paths )
    {
      String s{ path };
      s.replace( "\\", "/" );
      const int i{ s.find( "assets/" ) };
      TAC_ASSERT( i != s.npos );
      s = s.substr( i );
      const AssetPathString assetPath( s );
      result.push_back( assetPath );
    }
    return result;
  }

} // namespace Tac

bool Tac::Exists( const AssetPathStringView& assetPath )
{
  const UTF8Path fsPath( assetPath );
  return fsPath.Exists();
}

void Tac::SaveToFile( const AssetPathStringView& assetPath,
                      const void* bytes,
                      int byteCount,
                      Errors& errors )
{
  const UTF8Path fsPath( assetPath );
  fsPath.SaveToFile( bytes, byteCount, errors );
}

auto Tac::LoadAssetPath( const AssetPathStringView& assetPath, Errors& errors ) -> String
{
  const UTF8Path fsPath( assetPath );
  return fsPath.LoadFilePath( errors );
}


auto Tac::IterateAssetsDirs( const AssetPathStringView& dir,
                             AssetIterateType type,
                             Errors& errors ) -> AssetPathStrings
{
  const UTF8Path::IterateType fsIterate { AssetToFSIterateType( type ) };
  TAC_CALL_RET( const UTF8Paths paths{ UTF8Path( dir ).IterateDirectories( fsIterate, errors ) } );
  return ConvertAssetPaths( paths );
}

auto Tac::IterateAssetsInDir( const AssetPathStringView& dir,
                              AssetIterateType type,
                              Errors& errors ) -> AssetPathStrings
{
  const UTF8Path::IterateType fsIterate { AssetToFSIterateType( type ) };
  TAC_CALL_RET( const UTF8Paths paths{ UTF8Path( dir ).IterateFiles(  fsIterate, errors ) } );
  return ConvertAssetPaths( paths );
}

auto Tac::AssetOpenDialog( Errors& errors ) -> Tac::AssetPathStringView
{
  const UTF8Path dir { sInitialWorkingDir / sAssetPathRootFolderName };
  TAC_CALL_RET( const UTF8Path fsPath{ OS::OSOpenDialog(
    OpenParams{.mDefaultFolder { dir }, }, errors ) } );
  return ModifyPathRelative( fsPath, errors );
}

auto Tac::AssetSaveDialog( const AssetSaveDialogParams& params, Errors& errors ) -> Tac::AssetPathStringView
{
  const UTF8Path dir { sInitialWorkingDir / sAssetPathRootFolderName };
  const UTF8Path suggestedFilename { params.mSuggestedFilename };
  const UTF8Path fsPath { OS::OSSaveDialog(
    SaveParams
    {
      .mDefaultFolder     { dir },
      .mSuggestedFilename { suggestedFilename },
    }, errors ) };
  return ModifyPathRelative( fsPath, errors );
}

