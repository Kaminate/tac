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


  static std::filesystem::path StdPath( const char* s )
  {
    // Tell the filesystem path to interpret the input as UTF-8
    return std::filesystem::path( ( char8_t* )s );
  }

  static std::filesystem::path StdPath( const String& s )
  {
    return StdPath( s.c_str() );
  }

  static std::filesystem::path StdPath( const StringView& sv )
  {
    return StdPath( sv.c_str() );
  }

  static std::filesystem::path StdPath( const Path *path )
  {
    return StdPath( path->u8string() );
  }

  static std::filesystem::path StdPath( const Path &path )
  {
    return StdPath( path.u8string() );
  }

  static Path                  TacPath( const std::filesystem::path& p )
  {
    return Path( p.u8string().c_str() );
  }

  static String FormatFileError( const Path& path, const char* operation )
  {
    String errMsg; 
    errMsg += "Failed to open path (";
    errMsg += path.u8string().c_str();
    errMsg += ") for ";
    errMsg += operation;
    return errMsg;
  }

  template< typename T >
  static Paths IterateFilesHelperHelper( const T& iter, IterateFn isValid )
  {
    Paths paths;
    for( const std::filesystem::directory_entry& entry : iter )
      if( isValid( entry ) )
        paths.push_back( TacPath( entry.path() ) );
    return paths;
  }

  static Paths IterateFilesHelper( const Path& dir,
                                               IterateType type,
                                               IterateFn isValid,
                                               [[maybe_unused]] Errors& errors )
  {
    const std::filesystem::path stdPath{ StdPath( dir ) };
    if( type == IterateType::Default )
      return IterateFilesHelperHelper( std::filesystem::directory_iterator( stdPath ), isValid );

    if( type == IterateType::Recursive )
      return IterateFilesHelperHelper( std::filesystem::recursive_directory_iterator( stdPath ), isValid );

    return {};
  }

  // -----------------------------------------------------------------------------------------------

  // Constructors

  //Path::Path( const std::filesystem::path& path ) : mPath( path ) { }

  Path::Path( const char8_t* path ) : mUTF8String{ (const char*)path }{}
  Path::Path( const char* path ) : mUTF8String{ path }
    //: mUTF8String{ ( const char* )AsStdFilesystemPath( path ).u8string().c_str() }
  {
  }

  Path::Path( const String& path )                : Path( path.c_str() ) {}

  Path::Path( const StringView& path )            : Path( path.c_str() ) {}

  // -----------------------------------------------------------------------------------------------

  // Path Functions

  Path   Path::parent_path() const               { return TacPath(StdPath(this).parent_path()); }
  bool   Path::has_parent_path() const           { return StdPath(this).has_parent_path(); }

  Path   Path::filename() const                  { return TacPath(StdPath(this).filename()); }

  // [ ] Q: uhh yeah... what if it ends with a slash?
  // [ ] Q: this function is not part of the stl, how is it used in tac?
  Path   Path::dirname() const                   { return TacPath(StdPath(this).filename() ); }

  Path   Path::stem() const                      { return TacPath(StdPath(this).stem()); }
  bool   Path::has_stem() const                  { return StdPath(this).has_stem(); }

  String Path::u8string() const
  {
#if 0
    const std::u8string s0 = Get().u8string();
    const int n = ( int )s0.size();

    String s1;
    s1.resize( n );
    for( int i{}; i < n; ++i )
      s1[ i ] = s0[ i ];

    return s1;
#else
    return mUTF8String;
#endif
  }

  bool   Path::empty() const                     { return StdPath(this).empty(); }

  void   Path::clear()                           { return StdPath(this).clear(); }
  bool   Path::is_absolute() const               { return StdPath(this).is_absolute(); }
  bool   Path::is_relative() const               { return StdPath(this).is_relative(); }

  Path   Path::extension() const                 { return TacPath(StdPath(this).extension()); }
  bool   Path::has_extension() const             { return StdPath(this).has_extension(); }
  bool   Path::has_filename() const              { return StdPath(this).has_filename(); }
  bool   Path::has_dirname() const               { return StdPath(this).has_filename(); }
  //Path&  Path::operator /= ( const char* p )     { Get() /= p; return *this; }
  Path&  Path::operator /= ( const StringView& s ) { return *this = TacPath( StdPath(this) /= s.data() );  }
  Path&  Path::operator += ( const StringView& s ) { return *this = TacPath( StdPath(this) += s.data() );  }


  // -----------------------------------------------------------------------------------------------

  static std::filesystem::file_time_type StdTime( const Time time )
  {
    return *( std::filesystem::file_time_type* )time.mImpl;
  }

  static std::filesystem::file_time_type StdTime( const Time* time )
  {
    return *( std::filesystem::file_time_type* )time->mImpl;
  }

  static Time TacTime( std::filesystem::file_time_type stdTime )
  {
    Time tacTime;
    *( std::filesystem::file_time_type* )tacTime.mImpl = stdTime;
    return tacTime;
  }

  Time::Time()
    : mImpl { TAC_NEW std::filesystem::file_time_type }
  {
    //std::filesystem::file_time_type mTime{};
  }

  Time::~Time()
  {
    TAC_DELETE( std::filesystem::file_time_type* )mImpl;
  }

  // commenting out because std file_time_type doesnt have this, and
  // what does it mean to be "valid". time_since_epoch != 0?
  //bool Time::IsValid() const
  //{
  //  return StdTime(this) != Time{};
  //}


  //bool Time::operator == ( const Time& t ) const
  //{
  //  return StdTime(mTime.time_since_epoch() == t.mTime.time_since_epoch();
  //};

  //bool Time::operator != ( const Time& t ) const
  //{
  //  return mTime.time_since_epoch() != t.mTime.time_since_epoch();
  //};


  // -----------------------------------------------------------------------------------------------
}

bool Tac::FileSys::operator == ( Time a, Time b)
{
  return StdTime(a).time_since_epoch() == StdTime(b).time_since_epoch();
}

bool Tac::FileSys::operator != ( Time a, Time b)
{
  return StdTime(a).time_since_epoch() != StdTime(b).time_since_epoch();
}


namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  FileSys::Path  FileSys::operator / ( const Path& a, const Path& b )
  {
    return TacPath( StdPath(a) / StdPath( b ) );
  }

  bool              FileSys::operator == ( const Path& a, const Path& b )
  {
    return  StdPath( a ) == StdPath( b );
  }

  FileSys::Path  FileSys::GetCurrentWorkingDirectory()
  {
    return TacPath( std::filesystem::current_path() );
  }

  bool              FileSys::Exists( const Path& path )
  {
    return std::filesystem::exists( StdPath( path ) );
  }

  bool              FileSys::IsDirectory( const Path& path )
  {
    return std::filesystem::is_directory( StdPath( path ) );
  }

  bool              FileSys::Exists( const char* path )
  {
    return std::filesystem::exists( path );
  }

  void              FileSys::CreateDirectory( const Path& path )
  {
    std::filesystem::create_directory( StdPath( path) );
  }

  void              FileSys::CreateDirectory2( const Path& path )
  {
    std::filesystem::create_directory( StdPath( path) );
  }

  FileSys::Time  FileSys::GetFileLastModifiedTime( const FileSys::Path& path,
                                                         Errors& errors )
  {
    const std::filesystem::path stdPath{ StdPath( path ) };
    const std::filesystem::file_time_type stdTime{ std::filesystem::last_write_time( stdPath ) };
    return TacTime( stdTime );
  }

  static bool IsEntryDirectory( const std::filesystem::directory_entry& entry )
  {
     return entry.is_directory();
  };

  static bool  IsEntryRegularFile( const std::filesystem::directory_entry& entry )
  {
     return entry.is_regular_file(); 
  }

  FileSys::Paths FileSys::IterateFiles( const Path& dir, IterateType type, Errors& errors )
  {
    return IterateFilesHelper( dir, type,  &IsEntryRegularFile , errors );
  }


  FileSys::Paths FileSys::IterateDirectories( const Path& dir,
                                                    IterateType type,
                                                    Errors& errors )
  {
    return IterateFilesHelper( dir, type, &IsEntryDirectory, errors );
  }

  String            FileSys::StripExt( const StringView& path )
  {
    auto found { path.find_last_of( "." ) };
    TAC_ASSERT( found != String::npos );
    return path.substr( 0, found );
  }

  String            FileSys::StripLeadingSlashes( const StringView& path )
  {
    int i{};
    for( i; i < path.size(); i++ )
      if( path[ i ] != '/' && path[ i ] != '\\' )
        break;
    String result = path.substr( i );
    return result;
  }

  bool              FileSys::IsOfExt( const StringView& str, const StringView& ext )
  {
    String lower_str = ToLower( str );
    String lower_ext = ToLower( ext );
    return lower_str.ends_with( lower_ext );
  }

  void              FileSys::SaveToFile( const Path& path, StringView sv, Errors& errors )
  {
    SaveToFile( path, sv.data(), sv.size(), errors );
  }

  void              FileSys::SaveToFile( const Path& path,
                                            const void* bytes,
                                            int byteCount,
                                            Errors& errors )
  {
    const std::filesystem::path stdPath{ StdPath( path ) };
    std::ofstream ofs( stdPath, std::ios_base::binary | std::ios_base::out );
    TAC_RAISE_ERROR_IF( !ofs.is_open(), FormatFileError( path, "writing" ) );
    ofs.write( ( const char* )bytes, byteCount );
  }

  String            FileSys::LoadFilePath( const Path& path, Errors& errors )
  {
    const std::filesystem::path stdPath{ StdPath( path ) };
    std::ifstream ifs( stdPath, std::ios_base::binary );
    TAC_RAISE_ERROR_IF_RETURN( {}, !ifs.is_open() , FormatFileError( path, "reading" ) );
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
