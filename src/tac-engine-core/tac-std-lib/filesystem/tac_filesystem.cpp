#include "tac_filesystem.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/memory/tac_memory.h"

#if TAC_SHOULD_IMPORT_STD()
import std;
#else
#include <fstream>
//#include <chrono>
#include <filesystem>
#endif



namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  // Static helper functions

  using IterateFn = bool( * )( const std::filesystem::directory_entry& );

  static auto StdPath( const char* s )                                 { return std::filesystem::path( ( char8_t* )s ); }
  static auto StdPath( const String& s )                               { return std::filesystem::path( ( char8_t* )s.c_str() ); }
  static auto StdPath( const UTF8Path *path )                          { return std::filesystem::path( ( char8_t* )path->c_str() ); }
  static auto StdPath( const UTF8Path &path )                          { return std::filesystem::path( ( char8_t* )path.c_str() ); }
  static auto TacPath( const std::filesystem::path& p )                { return UTF8Path( ( char* )p.u8string().c_str() ); }

  static auto FormatFileError( const UTF8Path& path, const char* operation ) -> String
  {
    String errMsg; 
    errMsg += "Failed to open path (";
    errMsg += path;
    errMsg += ") for ";
    errMsg += operation;
    return errMsg;
  }

  template< typename T >
  static auto IterateFilesHelperHelper( const T& iter, IterateFn isValid ) -> UTF8Paths
  {
    UTF8Paths paths;
    for( const std::filesystem::directory_entry& entry : iter )
      if( isValid( entry ) )
        paths.push_back( TacPath( entry.path() ) );
    return paths;
  }

  static auto IterateFilesHelper( const UTF8Path& dir,
                                  UTF8Path::IterateType type,
                                  IterateFn isValid,
                                  [[maybe_unused]] Errors& errors ) -> UTF8Paths
  {
    const std::filesystem::path stdPath{ StdPath( dir ) };
    if( type == UTF8Path::IterateType::Default )
      return IterateFilesHelperHelper( std::filesystem::directory_iterator( stdPath ), isValid );

    if( type == UTF8Path::IterateType::Recursive )
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

  static auto StdTime( const FileTime& time ) -> const std::filesystem::file_time_type& { return *( std::filesystem::file_time_type* )time.mImpl; }
  static auto StdTime( dynmc FileTime& time ) -> dynmc std::filesystem::file_time_type& { return *( std::filesystem::file_time_type* )time.mImpl; }
  static auto StdTime( dynmc FileTime* time ) -> dynmc std::filesystem::file_time_type& { return *( std::filesystem::file_time_type* )time->mImpl; }
  static auto TacTime( std::filesystem::file_time_type stdTime ) { FileTime tacTime; StdTime( tacTime ) = stdTime; return tacTime; }



  // -----------------------------------------------------------------------------------------------

  // Constructors

  //Path::Path( const std::filesystem::path& path ) : mPath( path ) { }

  UTF8Path::UTF8Path( const char* path )                  : String( path ) {}
  UTF8Path::UTF8Path( const String& path )                : String( path ) {}
  UTF8Path::UTF8Path( const StringView& path )            : String( path ) {}

  // -----------------------------------------------------------------------------------------------

  // Path Functions

  auto UTF8Path::parent_path() const -> UTF8Path              { return TacPath(StdPath(this).parent_path()); }
  bool UTF8Path::has_parent_path() const                      { return StdPath(this).has_parent_path(); }


  // Note: both filename() and dirname() call std::filesystem::path::filename(),
  // which returns the last element in the path regardless of if its a file or a directory
  auto UTF8Path::filename() const -> UTF8Path                 { return TacPath(StdPath(this).filename()); }
  auto UTF8Path::dirname() const -> UTF8Path                  { return TacPath(StdPath(this).filename()); }

  auto UTF8Path::stem() const -> UTF8Path                     { return TacPath(StdPath(this).stem()); }
  bool UTF8Path::has_stem() const                             { return StdPath(this).has_stem(); }
  bool UTF8Path::is_absolute() const                          { return StdPath(this).is_absolute(); }
  bool UTF8Path::is_relative() const                          { return StdPath(this).is_relative(); }
  auto UTF8Path::extension() const -> UTF8Path                { return TacPath(StdPath(this).extension()); }
  bool UTF8Path::has_extension() const                        { return StdPath(this).has_extension(); }
  bool UTF8Path::has_filename() const                         { return StdPath(this).has_filename(); }
  bool UTF8Path::has_dirname() const                          { return StdPath(this).has_filename(); }


  void UTF8Path::SaveToFile(  StringView sv, Errors& errors ) const 
  {
    SaveToFile( sv.data(), sv.size(), errors );
  }

  void UTF8Path::SaveToFile( const void* bytes, int byteCount, Errors& errors ) const 
  {
    const std::filesystem::path stdPath{ StdPath( this ) };
    std::ofstream ofs( stdPath, std::ios_base::binary | std::ios_base::out );
    TAC_RAISE_ERROR_IF( !ofs.is_open(), FormatFileError( *this, "writing" ) );
    ofs.write( ( const char* )bytes, byteCount );
  }

  auto UTF8Path::LoadFilePath( Errors& errors ) const  -> String
  {
    const std::filesystem::path stdPath{ StdPath( this ) };
    std::ifstream ifs( stdPath, std::ios_base::binary );
    TAC_RAISE_ERROR_IF_RETURN( !ifs.is_open() , FormatFileError( *this, "reading" ) );
    ifs.seekg( 0, std::ifstream::end );
    const std::streampos byteCount{ ifs.tellg() };
    ifs.seekg( 0, std::ifstream::beg );
    String result;
    result.resize( ( int )byteCount );
    ifs.read( &result.front(), byteCount );
    return result;
  }

  auto UTF8Path::GetCurrentWorkingDirectory() -> UTF8Path                { return TacPath( std::filesystem::current_path() ); }
  bool UTF8Path::Exists() const      { return std::filesystem::exists( StdPath( this ) ); }
  bool UTF8Path::CreateDir() const   { return std::filesystem::create_directory( StdPath( this) ); }

  auto UTF8Path::GetFileLastModifiedTime(  Errors& errors )  const -> FileTime
  {
    TAC_UNUSED_PARAMETER( errors );
    const std::filesystem::path stdPath{ StdPath( this ) };
    const std::filesystem::file_time_type stdTime{ std::filesystem::last_write_time( stdPath ) };
    return TacTime( stdTime );
  }

  auto UTF8Path::IterateFiles(  IterateType type, Errors& errors )  const -> Vector<UTF8Path>
  {
    return IterateFilesHelper( *this, type,  &IsEntryRegularFile, errors );
  }

  auto UTF8Path::IterateDirectories( IterateType type, Errors& errors )  const -> Vector<UTF8Path>
  {
    return IterateFilesHelper( *this, type, &IsEntryDirectory, errors );
  }


  auto UTF8Path::operator /= ( const StringView& s ) -> UTF8Path& { return *this = TacPath( StdPath(this) /= s.data() );  }
  auto UTF8Path::operator += ( const StringView& s ) -> UTF8Path& { return *this = TacPath( StdPath(this) += s.data() );  }


  // -----------------------------------------------------------------------------------------------
  FileTime::FileTime( FileTime&& other ) noexcept : FileTime()    { SwapWith( ( FileTime&& )other ); }
  FileTime::FileTime() : mImpl                                    { TAC_NEW std::filesystem::file_time_type } {}
  FileTime::FileTime( const FileTime& other ) : FileTime()        { StdTime( this ) = StdTime( other ); }
  FileTime::~FileTime()                                           { TAC_DELETE( std::filesystem::file_time_type* )mImpl; }
  void FileTime::SwapWith( FileTime&& other ) noexcept            { Swap( mImpl, other.mImpl ); }
  void FileTime::operator = ( const FileTime& other )             { StdTime( this ) = StdTime( other ); }
  void FileTime::operator = ( FileTime&& other ) noexcept         { SwapWith( ( FileTime&& )other ); }

  // -----------------------------------------------------------------------------------------------
}



namespace Tac
{

  bool Tac::operator == ( FileTime a, FileTime b) { return StdTime(a).time_since_epoch() == StdTime(b).time_since_epoch(); }
  bool Tac::operator != ( FileTime a, FileTime b) { return StdTime(a).time_since_epoch() != StdTime(b).time_since_epoch(); }

  auto Tac::operator / ( const UTF8Path& a, const UTF8Path& b ) -> UTF8Path { return TacPath( StdPath(a) / StdPath( b ) ); }
  bool Tac::operator == ( const UTF8Path& a, const UTF8Path& b )            { return StdPath( a ) == StdPath( b ); }

} // namespace
