#include "tac_filesystem.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/memory/tac_memory.h"
//#include "tac-std-lib/os/tac_os.h"
//#include "tac-engine-core/asset/tac_asset.h"

#if TAC_SHOULD_IMPORT_STD()
import std;
#else
#include <fstream>
//#include <chrono>
#include <filesystem>
#endif



namespace Tac::FileSys
{
  // -----------------------------------------------------------------------------------------------

  // Static helper functions

  using IterateFn = bool( * )( const std::filesystem::directory_entry& );


  static auto StdPath( const char* s ) -> std::filesystem::path { return std::filesystem::path( ( char8_t* )s ); }
  static auto StdPath( const String& s ) -> std::filesystem::path { return StdPath( s.c_str() ); }
  static auto StdPath( const Path *path ) -> std::filesystem::path { return StdPath( path->u8string() ); }
  static auto StdPath( const Path &path ) -> std::filesystem::path { return StdPath( path.u8string() ); }
  static Path TacPath( const std::filesystem::path& p ) { return Path( p.u8string().c_str() ); }

  static auto FormatFileError( const Path& path, const char* operation ) -> String
  {
    String errMsg; 
    errMsg += "Failed to open path (";
    errMsg += path.u8string().c_str();
    errMsg += ") for ";
    errMsg += operation;
    return errMsg;
  }

  template< typename T >
  static auto IterateFilesHelperHelper( const T& iter, IterateFn isValid ) -> Paths
  {
    Paths paths;
    for( const std::filesystem::directory_entry& entry : iter )
      if( isValid( entry ) )
        paths.push_back( TacPath( entry.path() ) );
    return paths;
  }

  static auto IterateFilesHelper( const Path& dir,
                                  IterateType type,
                                  IterateFn isValid,
                                  [[maybe_unused]] Errors& errors ) -> Paths
  {
    const std::filesystem::path stdPath{ StdPath( dir ) };
    if( type == IterateType::Default )
      return IterateFilesHelperHelper( std::filesystem::directory_iterator( stdPath ), isValid );

    if( type == IterateType::Recursive )
      return IterateFilesHelperHelper( std::filesystem::recursive_directory_iterator( stdPath ), isValid );

    return {};
  }

  static bool IsEntryDirectory( const std::filesystem::directory_entry& entry )
  {
     return entry.is_directory();
  };

  static bool IsEntryRegularFile( const std::filesystem::directory_entry& entry )
  {
     return entry.is_regular_file(); 
  }


  // -----------------------------------------------------------------------------------------------

  // Constructors

  //Path::Path( const std::filesystem::path& path ) : mPath( path ) { }

  Path::Path( const char8_t* path )               : mUTF8String{ (const char*)path }{}
  Path::Path( const char* path )                  : mUTF8String{ path } {}
  Path::Path( const String& path )                : Path( path.c_str() ) {}
  Path::Path( const StringView& path )            : Path( path.c_str() ) {}

  // -----------------------------------------------------------------------------------------------

  // Path Functions

  Path Path::parent_path() const               { return TacPath(StdPath(this).parent_path()); }
  bool Path::has_parent_path() const           { return StdPath(this).has_parent_path(); }


  // Note: both filename() and dirname() call std::filesystem::path::filename(),
  // which returns the last element in the path regardless of if its a file or a directory
  Path Path::filename() const                  { return TacPath(StdPath(this).filename()); }
  Path Path::dirname() const                   { return TacPath(StdPath(this).filename()); }

  Path Path::stem() const                                 { return TacPath(StdPath(this).stem()); }
  bool Path::has_stem() const                             { return StdPath(this).has_stem(); }
  auto Path::u8string() const -> String                   { return mUTF8String; }
  bool Path::empty() const                                { return StdPath(this).empty(); }
  void Path::clear()                                      { mUTF8String.clear(); }
  bool Path::is_absolute() const                          { return StdPath(this).is_absolute(); }
  bool Path::is_relative() const                          { return StdPath(this).is_relative(); }
  Path Path::extension() const                            { return TacPath(StdPath(this).extension()); }
  bool Path::has_extension() const                        { return StdPath(this).has_extension(); }
  bool Path::has_filename() const                         { return StdPath(this).has_filename(); }
  bool Path::has_dirname() const                          { return StdPath(this).has_filename(); }
  auto Path::operator /= ( const StringView& s ) -> Path& { return *this = TacPath( StdPath(this) /= s.data() );  }
  auto Path::operator += ( const StringView& s ) -> Path& { return *this = TacPath( StdPath(this) += s.data() );  }


  // -----------------------------------------------------------------------------------------------

  static auto StdTime( const Time& time ) -> const std::filesystem::file_time_type& { return *( std::filesystem::file_time_type* )time.mImpl; }
  static auto StdTime( dynmc Time& time ) -> dynmc std::filesystem::file_time_type& { return *( std::filesystem::file_time_type* )time.mImpl; }
  static auto StdTime( dynmc Time* time ) -> dynmc std::filesystem::file_time_type& { return *( std::filesystem::file_time_type* )time->mImpl; }
  static Time TacTime( std::filesystem::file_time_type stdTime ) { Time tacTime; StdTime( tacTime ) = stdTime; return tacTime; }
  void Time::SwapWith( Time&& other ) noexcept    { Swap( mImpl, other.mImpl ); }
  Time::Time( Time&& other ) noexcept : Time()    { SwapWith( ( Time&& )other ); }
  Time::Time() : mImpl                            { TAC_NEW std::filesystem::file_time_type } {}
  Time::Time( const Time& other ) : Time()        { StdTime( this ) = StdTime( other ); }
  Time::~Time()                                   { TAC_DELETE( std::filesystem::file_time_type* )mImpl; }
  void Time::operator = ( const Time& other )     { StdTime( this ) = StdTime( other ); }
  void Time::operator = ( Time&& other ) noexcept { SwapWith( ( Time&& )other ); }

  // -----------------------------------------------------------------------------------------------
}

bool Tac::FileSys::operator == ( Time a, Time b) { return StdTime(a).time_since_epoch() == StdTime(b).time_since_epoch(); }
bool Tac::FileSys::operator != ( Time a, Time b) { return StdTime(a).time_since_epoch() != StdTime(b).time_since_epoch(); }


namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  auto FileSys::operator / ( const Path& a, const Path& b ) -> FileSys::Path { return TacPath( StdPath(a) / StdPath( b ) ); }
  bool FileSys::operator == ( const Path& a, const Path& b )                 { return StdPath( a ) == StdPath( b ); }
  auto FileSys::GetCurrentWorkingDirectory() -> FileSys::Path                { return TacPath( std::filesystem::current_path() ); }
  bool FileSys::Exists( const Path& path )      { return std::filesystem::exists( StdPath( path ) ); }
  bool FileSys::IsDirectory( const Path& path ) { return std::filesystem::is_directory( StdPath( path ) ); }
  bool FileSys::Exists( const char* path )      { return std::filesystem::exists( path ); }
  bool FileSys::CreateDir( const Path& path )   { return std::filesystem::create_directory( StdPath( path) ); }

  auto FileSys::GetFileLastModifiedTime( const FileSys::Path& path, Errors& errors ) -> FileSys::Time
  {
    TAC_UNUSED_PARAMETER( errors );
    const std::filesystem::path stdPath{ StdPath( path ) };
    const std::filesystem::file_time_type stdTime{ std::filesystem::last_write_time( stdPath ) };
    return TacTime( stdTime );
  }

  auto FileSys::IterateFiles( const Path& dir, IterateType type, Errors& errors ) -> FileSys::Paths
  {
    return IterateFilesHelper( dir, type,  &IsEntryRegularFile, errors );
  }

  auto FileSys::IterateDirectories( const Path& dir, IterateType type, Errors& errors ) -> FileSys::Paths
  {
    return IterateFilesHelper( dir, type, &IsEntryDirectory, errors );
  }

  auto FileSys::StripExt( const StringView& path ) -> String
  {
    auto found { path.find_last_of( "." ) };
    TAC_ASSERT( found != String::npos );
    return path.substr( 0, found );
  }

  auto FileSys::StripLeadingSlashes( const StringView& path ) -> String
  {
    int i{};
    for( i; i < path.size(); i++ )
      if( path[ i ] != '/' && path[ i ] != '\\' )
        break;
    String result = path.substr( i );
    return result;
  }

  bool FileSys::IsOfExt( const StringView& str, const StringView& ext )
  {
    String lower_str = ToLower( str );
    String lower_ext = ToLower( ext );
    return lower_str.ends_with( lower_ext );
  }

  void FileSys::SaveToFile( const Path& path, StringView sv, Errors& errors )
  {
    SaveToFile( path, sv.data(), sv.size(), errors );
  }

  void FileSys::SaveToFile( const Path& path,
                            const void* bytes,
                            int byteCount,
                            Errors& errors )
  {
    const std::filesystem::path stdPath{ StdPath( path ) };
    std::ofstream ofs( stdPath, std::ios_base::binary | std::ios_base::out );
    TAC_RAISE_ERROR_IF( !ofs.is_open(), FormatFileError( path, "writing" ) );
    ofs.write( ( const char* )bytes, byteCount );
  }

  auto FileSys::LoadFilePath( const Path& path, Errors& errors ) -> String
  {
    const std::filesystem::path stdPath{ StdPath( path ) };
    std::ifstream ifs( stdPath, std::ios_base::binary );
    TAC_RAISE_ERROR_IF_RETURN( !ifs.is_open() , FormatFileError( path, "reading" ) );
    ifs.seekg( 0, std::ifstream::end );
    const std::streampos byteCount{ ifs.tellg() };
    ifs.seekg( 0, std::ifstream::beg );
    String result;
    result.resize( ( int )byteCount );
    ifs.read( &result.front(), byteCount );
    return result;
  }

  // -----------------------------------------------------------------------------------------------

} // namespace
