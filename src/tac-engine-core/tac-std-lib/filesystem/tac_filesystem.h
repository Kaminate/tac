#pragma once

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

//import std; // <filesystem>

namespace Tac::FileSys
{
  // Minimal wrapper for std::filesystem.
  // Hard to roll your own because of OS formats (wchar_t) for example, and being able to handle
  // unicode in paths
  struct Path
  {
    // Constructors

    Path() = default;
    //Path( const std::filesystem::path& );
    Path( const char* );
    Path( const char8_t* );
    Path( const String& );
    Path( const StringView& );

    // ---------------------------------------------------------------------------------------------

    // Functions

    Path parent_path() const; // ie: foo/bar/qux.txt --> foo/bar
    Path stem() const;        // ie: foo/bar/qux.txt --> qux
    Path extension() const;   // ie: foo/bar/qux.txt --> .txt
    Path filename() const;    // ie: foo/bar/qux.txt --> qux.txt
    Path dirname() const;     // ie: foo/bar/qux.txt --> bar
    
    String u8string() const;  // convert to utf-8
    bool empty() const;
    void clear();

    bool is_absolute() const;
    bool is_relative() const;

    bool has_parent_path() const;
    bool has_stem() const;
    bool has_extension() const;
    bool has_filename() const;
    bool has_dirname() const;

    // In-class Operators


    //operator const std::filesystem::path &() const;
    //Path& operator /= ( const char* );
    Path& operator /= ( const StringView& );
    Path& operator = ( const Path& ) = default;
    Path& operator += ( const StringView& );

  private:
    String mUTF8String;
  };

  // Outside of class operators

  Path operator / ( const Path&, const Path& );
  bool operator == ( const Path&, const Path& );

  // -----------------------------------------------------------------------------------------------

  // -----------------------------------------------------------------------------------------------

  Path GetCurrentWorkingDirectory();

  bool Exists( const Path& );

  bool Exists( const char* );

  bool IsDirectory( const Path& );

#undef CreateDirectory
  void CreateDirectory( const Path& );
  void CreateDirectory2( const Path& );

  enum class IterateType { Default, Recursive };
  using Paths = Vector< Path >;

  Paths            IterateFiles( const Path& dir, IterateType, Errors& );
  Paths            IterateDirectories( const Path& dir, IterateType, Errors& );

  //
  // Filesystem utils
  //

  bool   IsOfExt( const StringView& str, const StringView& ext );
  String StripExt( const StringView& ); // "foo.txt" --> "foo"
  String StripLeadingSlashes( const StringView& );

  // -----------------------------------------------------------------------------------------------

  // File I/O

  void        SaveToFile( const Path&, StringView, Errors& );
  void        SaveToFile( const Path&, const void*, int, Errors& );
  String      LoadFilePath( const Path&, Errors& );

  // not using time_t anymore, just using the default clock cppstl filesystem comes with
  struct Time
  {
    Time();
    ~Time();
    //bool IsValid() const;
    //bool operator == ( const Time& ) const;
    //bool operator != ( const Time& ) const;

    void* mImpl;
    //std::filesystem::file_time_type mTime{};
  };

  bool operator == ( Time, Time );
  bool operator != ( Time, Time );

  Time GetFileLastModifiedTime( const Path&, Errors& );

  // -----------------------------------------------------------------------------------------------

  //void DeleteThisFilesystemFn();

} // namespace Tac::Filesystem
