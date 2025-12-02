#pragma once

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  struct FileTime
  {
    FileTime();
    FileTime( const FileTime& );
    FileTime( FileTime&& ) noexcept;
    ~FileTime();
    void SwapWith( FileTime&& ) noexcept;
    void operator = ( const FileTime& );
    void operator = ( FileTime&& ) noexcept;
    void* mImpl{};
  };

  struct UTF8Path : public String
  {
    enum class IterateType { Default, Recursive };
    UTF8Path() = default;
    UTF8Path( const char* );
    UTF8Path( const String& );
    UTF8Path( StringView );
    auto parent_path() const -> UTF8Path; // ie: foo/bar/qux.txt --> foo/bar
    auto stem() const -> UTF8Path;        // ie: foo/bar/qux.txt --> qux
    auto extension() const -> UTF8Path;   // ie: foo/bar/qux.txt --> .txt
    auto filename() const -> UTF8Path;    // ie: foo/bar/qux.txt --> qux.txt
    auto dirname() const -> UTF8Path;     // ie: foo/bar/qux.txt --> bar
    bool is_absolute() const;
    bool is_relative() const;
    bool has_parent_path() const;
    bool has_stem() const;
    bool has_extension() const;
    bool has_filename() const;
    bool has_dirname() const;
    bool Exists() const;
    bool CreateDir() const;
    auto IterateFiles( IterateType, Errors& ) const -> Vector< UTF8Path >;
    auto IterateDirectories( IterateType, Errors& ) const  -> Vector< UTF8Path >;
    void SaveToFile( StringView, Errors& ) const ;
    void SaveToFile( const void*, int, Errors& ) const ;
    auto LoadFilePath( Errors& ) const -> String;
    auto GetFileLastModifiedTime( Errors& ) const -> FileTime;
    auto operator /= ( StringView ) -> UTF8Path&;
    auto operator = ( const UTF8Path& ) ->UTF8Path& = default;
    auto operator += ( StringView ) -> UTF8Path&;
    static auto GetCurrentWorkingDirectory() -> UTF8Path;
  };

  using UTF8Paths = Vector< UTF8Path >;


  auto operator / ( const UTF8Path&, const UTF8Path& ) -> UTF8Path;
  bool operator == ( const UTF8Path&, const UTF8Path& );
  bool operator == ( FileTime, FileTime );
  bool operator != ( FileTime, FileTime );
} // namespace Tac::FileSys
