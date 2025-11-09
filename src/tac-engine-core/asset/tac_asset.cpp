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

  static const FileSys::Path sInitialWorkingDir       { FileSys::GetCurrentWorkingDirectory() };
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

  static bool IsValid( const StringView& s )
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

  static void Validate( const StringView& s )
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

  static FileSys::IterateType AssetToFSIterateType( AssetIterateType t )
  {
    switch( t )
    {
    case AssetIterateType::Default: return FileSys::IterateType::Default;
    case AssetIterateType::Recursive: return FileSys::IterateType::Recursive;
    default: TAC_ASSERT_INVALID_CASE( t ); return FileSys::IterateType::Default;
    }
  }

  // -----------------------------------------------------------------------------------------------

  // AssetPathStringView

  AssetPathStringView::AssetPathStringView( const char* s ) : StringView( s )                       { Validate( s ); }
  AssetPathStringView::AssetPathStringView( const String& s ) : StringView( s )                     { Validate( s ); }
  AssetPathStringView::AssetPathStringView( const AssetPathString& s ) : StringView( s )            {} // already validated
  AssetPathStringView::AssetPathStringView( const StringView& s ) : StringView( s )                 { Validate( s ); };

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


  static auto ModifyPathRelative( const FileSys::Path& path, Errors& errors ) -> Tac::AssetPathStringView
  {
    const String workingUTF8 { sInitialWorkingDir.u8string() };
    dynmc String pathUTF8 { path.u8string() };
    if( path.is_absolute() )
    {
      TAC_RAISE_ERROR_IF_RETURN( !pathUTF8.starts_with( workingUTF8 ), 
                                 String() + pathUTF8 + String( " is not in " ) + workingUTF8 );
      pathUTF8.erase( 0, workingUTF8.size() );
      pathUTF8 = FileSys::StripLeadingSlashes( pathUTF8 );
    }

    pathUTF8.replace( "\\", "/");
    return FrameMemoryCopy( pathUTF8.c_str() );
  }

  static AssetPathStrings ConvertAssetPaths( const FileSys::Paths& paths )
  {
    AssetPathStrings result;
    for( const FileSys::Path& path : paths )
    {
      String s{ path.u8string() };
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
  const FileSys::Path fsPath( assetPath );
  return Exists( fsPath );
}

void Tac::SaveToFile( const AssetPathStringView& assetPath,
                      const void* bytes,
                      int byteCount,
                      Errors& errors )
{
  const FileSys::Path fsPath( assetPath );
  FileSys::SaveToFile( fsPath, bytes, byteCount, errors );
}

auto Tac::LoadAssetPath( const AssetPathStringView& assetPath, Errors& errors ) -> String
{
  const FileSys::Path fsPath( assetPath );
  return FileSys::LoadFilePath( fsPath, errors );
}


auto Tac::IterateAssetsDirs( const AssetPathStringView& dir,
                             AssetIterateType type,
                             Errors& errors ) -> AssetPathStrings
{
  const FileSys::IterateType fsIterate { AssetToFSIterateType( type ) };
  TAC_CALL_RET( const FileSys::Paths paths{ FileSys::IterateDirectories( dir, fsIterate, errors ) } );
  return ConvertAssetPaths( paths );
}

auto Tac::IterateAssetsInDir( const AssetPathStringView& dir,
                              AssetIterateType type,
                              Errors& errors ) -> AssetPathStrings
{
  const FileSys::IterateType fsIterate { AssetToFSIterateType( type ) };
  TAC_CALL_RET( const FileSys::Paths paths{ FileSys::IterateFiles( dir, fsIterate, errors ) } );
  return ConvertAssetPaths( paths );
}

auto Tac::AssetOpenDialog( Errors& errors ) -> Tac::AssetPathStringView
{
  const FileSys::Path dir { sInitialWorkingDir / sAssetPathRootFolderName };
  TAC_CALL_RET( const FileSys::Path fsPath{ OS::OSOpenDialog(
    OS::OpenParams{.mDefaultFolder { &dir }, }, errors ) } );
  return ModifyPathRelative( fsPath, errors );
}

auto Tac::AssetSaveDialog( const AssetSaveDialogParams& params, Errors& errors ) -> Tac::AssetPathStringView
{
  const FileSys::Path dir { sInitialWorkingDir / sAssetPathRootFolderName };
  const FileSys::Path suggestedFilename { params.mSuggestedFilename };
  const FileSys::Path fsPath { OS::OSSaveDialog(
    OS::SaveParams
    {
      .mDefaultFolder     { & dir },
      .mSuggestedFilename { &suggestedFilename },
    }, errors ) };
  return ModifyPathRelative( fsPath, errors );
}

