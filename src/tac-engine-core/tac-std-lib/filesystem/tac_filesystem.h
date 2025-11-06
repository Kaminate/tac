#pragma once

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac::FileSys
{
  // Minimal wrapper for std::filesystem.
  // Hard to roll your own because of OS formats (wchar_t) for example, and being able to handle
  // unicode in paths
  struct Path
  {
    Path() = default;
    Path( const char* );
    Path( const char8_t* );
    Path( const String& );
    Path( const StringView& );
    Path parent_path() const; // ie: foo/bar/qux.txt --> foo/bar
    Path stem() const;        // ie: foo/bar/qux.txt --> qux
    Path extension() const;   // ie: foo/bar/qux.txt --> .txt
    Path filename() const;    // ie: foo/bar/qux.txt --> qux.txt
    Path dirname() const;     // ie: foo/bar/qux.txt --> bar
    auto u8string() const -> String;  // convert to utf-8
    bool empty() const;
    void clear();
    bool is_absolute() const;
    bool is_relative() const;
    bool has_parent_path() const;
    bool has_stem() const;
    bool has_extension() const;
    bool has_filename() const;
    bool has_dirname() const;
    auto operator /= ( const StringView& ) -> Path&;
    auto operator = ( const Path& ) ->Path& = default;
    auto operator += ( const StringView& ) -> Path&;

  private:
    String mUTF8String;
  };

  // Outside of class operators

  Path operator / ( const Path&, const Path& );
  bool operator == ( const Path&, const Path& );

  enum class IterateType { Default, Recursive };
  using Paths = Vector< Path >;

  Path GetCurrentWorkingDirectory();
  bool Exists( const Path& );
  bool Exists( const char* );
  bool IsDirectory( const Path& );
  bool CreateDir( const Path& );
  auto IterateFiles( const Path& dir, IterateType, Errors& ) -> Paths;
  auto IterateDirectories( const Path& dir, IterateType, Errors& ) -> Paths;
  bool IsOfExt( const StringView& str, const StringView& ext );
  auto StripExt( const StringView& ) -> String; // "foo.txt" --> "foo"
  auto StripLeadingSlashes( const StringView& ) -> String;
  void SaveToFile( const Path&, StringView, Errors& );
  void SaveToFile( const Path&, const void*, int, Errors& );
  auto LoadFilePath( const Path&, Errors& ) -> String;

  // not using time_t anymore, just using the default clock cppstl filesystem comes with
  struct Time
  {
    Time();
    Time( const Time& );
    Time( Time&& ) noexcept;
    ~Time();
    void operator = ( const Time& );
    void operator = ( Time&& ) noexcept;
    void SwapWith( Time&& ) noexcept;
    void* mImpl{};
  };

  bool operator == ( Time, Time );
  bool operator != ( Time, Time );

  Time GetFileLastModifiedTime( const Path&, Errors& );

} // namespace Tac::FileSys
