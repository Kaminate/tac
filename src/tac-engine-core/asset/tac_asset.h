#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/meta/tac_meta_decl.h"

namespace Tac
{
  struct Errors;
  
  // An asset path
  // - is a local path to an asset
  // - starts with "assets/", or is an empty string
  // - may be a path to a file, or a path to a directory
  // - uses ascii encoding (no utf16)
  // - is easily convertable to a StringID
  // - Only uses forward slash '/', does not use '\\'
  // - As a directory, does not end with a slash <-- under debate
  // - As a file, always has an extension
  //
  // Because an asset path can be initialized from a c-string literal (hardcoded)
  // or loaded from a file (data driven), this class inherits from StringView

  struct AssetPathStringView;


  inline const char     AssetPathSeperator      { '/' };
  inline const char*    AssetPathRootFolderName { "assets" };

  struct AssetPathString : public String
  {
    AssetPathString() = default;
    AssetPathString( const AssetPathStringView& );
    void operator = ( const String& );
  };

  struct AssetPathStringView : public StringView
  {
    AssetPathStringView() = default;
    AssetPathStringView( const char* );
    AssetPathStringView( const String& );
    AssetPathStringView( const StringView& );
    AssetPathStringView( const AssetPathString& );
    AssetPathStringView GetDirectory() const;
    StringView          GetFilename() const; // "foo/bar.qux" --> "bar.qux"
    StringView          GetFileExtension() const; // ".png", ".txt"
    bool                IsFile() const;
    bool                IsDirectory() const;
  };

  using AssetPathStrings = Vector< AssetPathString >;
  enum class AssetIterateType { Default, Recursive };

  void             SaveToFile( const AssetPathStringView&, const void*, int, Errors& );
  String           LoadAssetPath( const AssetPathStringView&, Errors& );
  bool             Exists( const AssetPathStringView& );
  AssetPathStrings IterateAssetsInDir( const AssetPathStringView&, AssetIterateType, Errors& );

  TAC_META_DECL( AssetPathString );

} // namespace Tac
