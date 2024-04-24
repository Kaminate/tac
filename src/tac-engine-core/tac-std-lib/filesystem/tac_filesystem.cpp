#include "tac_filesystem.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
//#include "tac-std-lib/os/tac_os.h"
//#include "tac-std-lib/filesystem/tac_asset.h"

//#include <fstream>
//#include <chrono>
//#include <filesystem>


//namespace Tac
//{
//  // Time stuff TODO MOVE INTO DIFFERENT FILE
//
//  struct Time::Impl { std::time_t mTime{}; };
//
//  Time::Time() : mImpl( TAC_NEW Impl ) {}
//
//  Time::~Time() { TAC_DELETE mImpl; }
//
//  Time 
//}

namespace Tac::Filesystem
{
  // -----------------------------------------------------------------------------------------------

  // Static helper functions

  using IterateFn = bool( * )( const std::filesystem::directory_entry& );

  static String FormatFileError( const Path& path, const char* operation )
  {
    std::stringstream ss;
    ss << "Failed to open path for ";
    ss << operation;
    ss << path.Get();
    String errmsg( ss.str().c_str() );
    return errmsg;
  }

  template< typename T >
  static Paths IterateFilesHelperHelper( const T& iter, IterateFn isValid )
  {
    Paths paths;
    for( const std::filesystem::directory_entry& entry : iter )
      if( isValid( entry ) )
        paths.push_back( entry.path() );
    return paths;
  }

  static Paths IterateFilesHelper( const Path& dir,
                                               IterateType type,
                                               IterateFn isValid,
                                               [[maybe_unused]] Errors& errors )
  {
    if( type == IterateType::Default )
      return IterateFilesHelperHelper( std::filesystem::directory_iterator( dir.Get() ), isValid );

    if( type == IterateType::Recursive )
      return IterateFilesHelperHelper( std::filesystem::recursive_directory_iterator( dir.Get() ), isValid );

    return {};
  }

  // -----------------------------------------------------------------------------------------------

  // Constructors

  Path::Path( const std::filesystem::path& path ) : mPath( path ) { }

  Path::Path( const char* path )                  : mPath( path ) {}

  Path::Path( const String& path )                : Path( path.c_str() ) {}

  Path::Path( const StringView& path )            : Path( path.c_str() ) {}


  // -----------------------------------------------------------------------------------------------

  // Path Functions

  Path   Path::parent_path() const               { return Get().parent_path(); }
  bool   Path::has_parent_path() const           { return mPath.has_parent_path(); }

  Path   Path::filename() const                  { return Get().filename(); }

  Path   Path::dirname() const
  {
    // uhh yeah... what if it ends with a slash?
    // [ ] Q: this function is not part of the stl, how is it used in tac?
    return Get().filename();
  }

  Path   Path::stem() const                      { return mPath.stem(); }
  bool   Path::has_stem() const                  { return mPath.has_stem(); }

  String Path::u8string() const
  {
    const std::u8string s0 = Get().u8string();
    const int n = ( int )s0.size();

    String s1;
    s1.resize( n );
    for( int i{}; i < n; ++i )
      s1[ i ] = s0[ i ];

    return s1;
  }

  bool   Path::empty() const                     { return Get().empty(); }

  void   Path::clear()                           { return Get().clear(); }
  bool   Path::is_absolute() const               { return Get().is_absolute(); }
  bool   Path::is_relative() const               { return Get().is_relative(); }

  Path   Path::extension() const                 { return Get().extension(); }
  bool   Path::has_extension() const             { return Get().has_extension(); }
  bool   Path::has_filename() const              { return Get().has_filename(); }
  bool   Path::has_dirname() const               { return Get().has_filename(); }
  //Path&  Path::operator /= ( const char* p )     { Get() /= p; return *this; }
  Path&  Path::operator /= (const StringView& s ) { Get() /= s.data(); return *this; }

  std::filesystem::path&       Path::Get()       { return mPath; }
  const std::filesystem::path& Path::Get() const { return mPath; }

  // -----------------------------------------------------------------------------------------------

  bool Time::IsValid() const
  {
    return *this != Time{};
  }

  bool Time::operator == ( const Time& t ) const
  {
    return mTime.time_since_epoch() == t.mTime.time_since_epoch();
  };

  bool Time::operator != ( const Time& t ) const
  {
    return mTime.time_since_epoch() != t.mTime.time_since_epoch();
  };


  // -----------------------------------------------------------------------------------------------
}

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  Filesystem::Path  Filesystem::operator / ( const Path& a, const Path& b )
  {
    return a.Get() / b.Get();
  }

  bool              Filesystem::operator == ( const Path& a, const Path& b )
  {
    return a.Get() == b.Get();
  }

  Filesystem::Path  Filesystem::GetCurrentWorkingDirectory()
  {
    return std::filesystem::current_path();
  }

  bool              Filesystem::Exists( const Path& path )
  {
    const std::filesystem::path& fsPath = path.Get();
    return std::filesystem::exists( fsPath );
  }

  bool              Filesystem::IsDirectory( const Path& path )
  {
    return std::filesystem::is_directory( path.Get() );
  }

  bool              Filesystem::Exists( const char* path )
  {
    return std::filesystem::exists( path );
  }

  void              Filesystem::CreateDirectory( const Path& path )
  {
    std::filesystem::create_directory( path.Get() );
  }

  void              Filesystem::CreateDirectory2( const Path& path )
  {
    std::filesystem::create_directory( path.Get() );
  }

  Filesystem::Time  Filesystem::GetFileLastModifiedTime( const Filesystem::Path& path,
                                                         Errors& errors )
  {
    const std::filesystem::path& fsPath = path.Get();
    const auto time = std::filesystem::last_write_time( fsPath );
    return Time{ .mTime = time, };
  }

  Filesystem::Paths Filesystem::IterateFiles( const Path& dir, IterateType type, Errors& errors )
  {
    IterateFn isValid = []( const auto& entry ) { return entry.is_regular_file(); };
    return IterateFilesHelper( dir, type, isValid, errors );
  }

  Filesystem::Paths Filesystem::IterateDirectories( const Path& dir,
                                                    IterateType type,
                                                    Errors& errors )
  {
    IterateFn isValid { []( const std::filesystem::directory_entry& entry ) { return entry.is_directory() }; };
    return IterateFilesHelper( dir, type, isValid, errors );
  }

  String            Filesystem::StripExt( const StringView& path )
  {
    auto found { path.find_last_of( "." ) };
    TAC_ASSERT( found != String::npos );
    return path.substr( 0, found );
  }

  String            Filesystem::StripLeadingSlashes( const StringView& path )
  {
    int i;
    for( i = 0; i < path.size(); i++ )
      if( path[ i ] != '/' && path[ i ] != '\\' )
        break;
    String result = path.substr( i );
    return result;
  }

  bool              Filesystem::IsOfExt( const StringView& str, const StringView& ext )
  {
    String lower_str = ToLower( str );
    String lower_ext = ToLower( ext );
    return lower_str.ends_with( lower_ext );
  }

  void              Filesystem::SaveToFile( const Path& path, StringView sv, Errors& errors )
  {
    SaveToFile( path, sv.data(), sv.size(), errors );
  }

  void              Filesystem::SaveToFile( const Path& path,
                                            const void* bytes,
                                            int byteCount,
                                            Errors& errors )
  {
    std::ofstream ofs( path.Get(), std::ios_base::binary | std::ios_base::out );
    TAC_RAISE_ERROR_IF( !ofs.is_open(), FormatFileError( path, "writing" ) );
    ofs.write( ( const char* )bytes, byteCount );
  }

  String            Filesystem::LoadFilePath( const Path& path, Errors& errors )
  {
    std::ifstream ifs(  path.Get(), std::ios_base::binary );
    TAC_RAISE_ERROR_IF_RETURN( !ifs.is_open() , FormatFileError( path, "reading" ),  {} );
    ifs.seekg( 0, std::ifstream::end );
    std::streampos byteCount = ifs.tellg();
    ifs.seekg( 0, std::ifstream::beg );
    String result;
    result.resize( ( int )byteCount );
    ifs.read( &result.front(), byteCount );
    return result;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace
