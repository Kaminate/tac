#include "tac-std-lib/dataprocess/tac_settings.h" // self-inc

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "src/shell/tac_desktop_app.h"

//#include <filesystem>

namespace Tac
{
  static Json mJson;
  static bool dirty;
  static bool flush;
  static Timestamp lastSaveSeconds;
  static Filesystem::Path savePath;

  static void   SettingCheckFallback( Json* leaf, Json* fallback)
  {
    if( leaf->mType != fallback->mType )
    {
      *leaf = *fallback;
      SettingsSave();
    }
  }

  static void   SettingsSetValue( StringView path, Json* root, const Json& setValue)
  {
    *SettingsGetJson( path, root ) = setValue;
    SettingsSave();
  }

  static Json*  SettingsGetValue( StringView path, Json* fallback, Json* root)
  {
    Json* leaf = SettingsGetJson( path, root );
    SettingCheckFallback( leaf, fallback);
    return leaf;
  }

  void          SettingsInit( Errors& errors )
  {
    savePath = sShellPrefPath / ( sShellAppName + "Settings.txt" );
    if( Filesystem::Exists( savePath ))
    {
      const String loaded = LoadFilePath( savePath, errors );
      mJson.Parse( loaded.data(), ( int )loaded.size(), errors );
    }
    else
    {
      SettingsFlush( errors );
    }
  }


  void SettingsTick( Errors& errors )
  {
    const Timestamp elapsedSeconds = Timestep::GetElapsedTime();
    const TimestampDifference saveFrequencySecs = 0.1f;
    if( !flush )
    {
      if( dirty )
      {
        const bool savedRecently = elapsedSeconds < lastSaveSeconds + saveFrequencySecs;
        if( savedRecently )
          return;
      }
      else
      {
        return;
      }
    }

    const String str = mJson.Stringify();
    const void* data = ( void* )str.data();
    const int n = ( int )str.size();
    
    TAC_CALL( Filesystem::SaveToFile( savePath, data, n, errors ));

    dirty = false;
    flush = false;
    lastSaveSeconds = elapsedSeconds;
  }

  void          SettingsSave()
  {
    dirty = true;
  }

  void       SettingsFlush( Errors& e )
  {
    flush = true;
    SettingsTick( e );
  }

  Json*         SettingsGetChildByKeyValuePair( StringView key, const Json& value, Json* root)
  {
    TAC_ASSERT( root );
    root->mType = root->mType == JsonType::Null ? JsonType::Array : root->mType;
    TAC_ASSERT( root->mType == JsonType::Array );
    TAC_ASSERT( value.mType == JsonType::String ||
                value.mType == JsonType::Number ||
                value.mType == JsonType::Bool );
    for( Json* child : root->mArrayElements )
    {
      if( !child->HasChild( key ) )
        continue;
      Json& childValue = child->GetChild( key );
      if( childValue.mType != value.mType ||
          childValue.mString != value.mString ||
          childValue.mNumber != value.mNumber ||
          childValue.mBoolean != value.mBoolean )
        continue;
      return child;
    }

    Json* child = root->AddChild();
    SettingsSetValue( key, child, value);
    return child;
  }

  Json*         SettingsGetJson( StringView path, Json* root )
  {
    root = root ? root : &mJson;
    while( !path.empty() )
    {
      // Non-leaf nodes are either JsonType::Objects/Arrays
      StringView oldPath = path;
      Json* oldRoot = root;
      if( path.front() == '.' )
        path.remove_prefix( 1 );

      if( IsAlpha( path.front() ) )
      {
        root->mType = root->mType == JsonType::Null ? JsonType::Object : root->mType;
        TAC_ASSERT( root->mType == JsonType::Object );
        const char* keyEnd = path.data();
        while( keyEnd < path.end() &&
               ( IsSpace( *keyEnd ) ||
               IsAlpha( *keyEnd ) ||
               IsDigit( *keyEnd ) ||
               *keyEnd == '_' ) )
          keyEnd++;
        const StringView key( path.data(), keyEnd );
        path.remove_prefix( key.size() );
        root = &root->GetChild( key );
      }
      else if( path.front() == '[' )
      {
        root->mType = root->mType == JsonType::Null ? JsonType::Array : root->mType;
        TAC_ASSERT( root->mType == JsonType::Array );
        const int iCloseSquareBracket = path.find_first_of( "]" );
        TAC_ASSERT( iCloseSquareBracket != StringView::npos );
        const int iElement = Atoi( StringView( path.data() + 1,
                                   path.data() + iCloseSquareBracket ) );
        TAC_ASSERT( ( unsigned )iElement < ( unsigned )root->mArrayElements.size() );
        root = root->mArrayElements[ iElement ];
        path.remove_prefix( iCloseSquareBracket );
      }

      TAC_ASSERT( oldRoot != root );
      TAC_ASSERT( StrCmp( oldPath, path ) );
    }
    TAC_ASSERT( root != &mJson );
    return root;
  }

  StringView    SettingsGetString( StringView path, StringView fallback, Json* root)
  {
    Json fallbackJson( fallback );
    return SettingsGetValue( path, &fallbackJson, root)->mString;
  }

  void          SettingsSetString( StringView path, StringView setValue, Json* root)
  {
    SettingsSetValue( path, root, setValue );
  }

  JsonNumber    SettingsGetNumber( StringView path, JsonNumber fallback, Json* root )
  {
    Json fallbackJson( fallback );
    return SettingsGetValue( path, &fallbackJson, root )->mNumber;
  }

  void          SettingsSetNumber( StringView path, JsonNumber setValue, Json* root )
  {
    SettingsSetValue( path, root, setValue );
  }

  bool          SettingsGetBool( StringView path, bool fallback, Json* root )
  {
    Json fallbackJson( fallback );
    return SettingsGetValue( path, &fallbackJson, root )->mBoolean;
  }

  void          SettingsSetBool( StringView path, bool setValue, Json* root )
  {
    SettingsSetValue( path, root, setValue );
  }

}
