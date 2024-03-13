#pragma once

//#include "tac-std-lib/tac_core.h"
#include "tac-std-lib/containers/tac_vector.h"

import std; // <filesystem>, <ctime> ( std::time_t )


namespace Tac { struct String; struct StringView; struct Errors; }

namespace Tac::Filesystem
{
  // Minimal wrapper for std::filesystem.
  // Hard to roll your own because of OS formats (wchar_t) for example, and being able to handle
  // unicode in paths
  struct Path
  {
    // Constructors

    Path() = default;
    Path( const std::filesystem::path& );
    Path( const char* );
    Path( const String& );
    Path( const StringView& );

    // ---------------------------------------------------------------------------------------------

    // Functions

    Path parent_path() const;
    Path stem() const; // ie: foo/bar.txt --> bar
    Path extension() const; // includes the '.'
    Path filename() const; // ie: foo/bar.txt --> bar.txt
    Path dirname() const;
    
    String u8string() const; // convert to utf-8
    bool empty() const;
    void clear();

    bool is_absolute() const;
    bool is_relative() const;

    bool has_parent_path() const;
    bool has_stem() const;
    bool has_extension() const;
    bool has_filename() const;
    bool has_dirname() const;

    std::filesystem::path& Get();
    const std::filesystem::path& Get() const;

    // In-class Operators


    //operator const std::filesystem::path &() const;
    Path& operator /= ( const char* );
    Path& operator = ( const Path& ) = default;

  private:


    // Data

    std::filesystem::path mPath;
  };

  // Outside of class operators

  Path operator / ( const Path&, const Path& );
  bool operator == ( const Path&, const Path& );

  // -----------------------------------------------------------------------------------------------

  // -----------------------------------------------------------------------------------------------

  Path GetCurrentWorkingDirectory();

  [[nodiscard]]
  bool Exists( const Path& );

  [[nodiscard]]
  bool Exists( const char* );

  bool IsDirectory( const Path& );

  void CreateDirectory( const Path& );
  void CreateDirectory2( const Path& );

  enum class IterateType { Default, Recursive };
  using Paths = Vector<Path>;

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
  //void        SaveToFile( const char*, const void*, int, Errors& );
  String      LoadFilePath( const Path&, Errors& );

  // not using time_t anymore, just using the default clock cppstl filesystem comes with
  struct Time
  {
    bool IsValid() const;
    bool operator == ( const Time& ) const;
    bool operator != ( const Time& ) const;

    std::filesystem::file_time_type mTime{};
  };

  Time GetFileLastModifiedTime( const Path&, Errors& );

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Filesystem
