#include "src/common/tacSettings.h"
#include "src/common/tacUtility.h"
#include "src/common/tacMemory.h"
#include "src/common/tacOS.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacShell.h"
#include "src/shell/tacDesktopApp.h"

namespace Tac
{
  static Json mJson;



  static String SettingsGetSavePath()
  {
    StringView appName = Shell::Instance.mAppName;
    StringView prefPath = Shell::Instance.mPrefPath;
    String path = prefPath + "/" + appName + "Settings.txt";
    return path;
  }

  static void SettingCheckFallback( Json* leaf, Json* fallback )
  {
    if( leaf->mType != fallback->mType )
    {
      *leaf = *fallback;
      SettingsSave( Errors() );
    }
  }

  static void SettingsSetValue( StringView path, Json* root, const Json& setValue )
  {
    *SettingsGetJson( path, root ) = setValue;
    SettingsSave( Errors() );
  }

  static Json* SettingsGetValue( StringView path, Json* fallback, Json* root )
  {
    Json* leaf = SettingsGetJson( path, root );
    SettingCheckFallback( leaf, fallback );
    return leaf;
  }

  void       SettingsInit( Errors& errors )
  {
    if( !FileExist( SettingsGetSavePath() ) )
    {
      SettingsSave( errors );
      TAC_HANDLE_ERROR( errors );
      return;
    }
    auto temporaryMemory = TemporaryMemoryFromFile( SettingsGetSavePath(), errors );
    mJson.Parse( temporaryMemory.data(), ( int )temporaryMemory.size(), errors );
  }

  void       SettingsSave( Errors& errors )
  {
    String str = mJson.Stringify();
    OS::SaveToFile( SettingsGetSavePath(), ( void* )str.data(), ( int )str.size(), errors );
    TAC_HANDLE_ERROR( errors );
  }

  Json*      SettingsGetChildByKeyValuePair( StringView key, const Json& value, Json* root )
  {
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
    SettingsSetValue( key, child, value );
    return child;
  }

  Json*      SettingsGetJson( StringView path, Json* root )
  {
    root = root ? root : &mJson;
    while( !path.empty() )
    {
      // Non-leaf nodes are either JsonType::Objects/Arrays
      StringView oldPath = path;
      Json* oldRoot = root;
      if( path.starts_with( "." ) )
        path.remove_prefix( 1 );
      if( IsAlpha( *path.data() ) )
      {
        root->mType = root->mType == JsonType::Null ? JsonType::Object : root->mType;
        TAC_ASSERT( root->mType == JsonType::Object );
        const char* keyEnd = path.data();
        while( keyEnd < path.end() && ( IsSpace( *keyEnd ) || IsAlpha( *keyEnd ) ) )
          keyEnd++;
        const StringView key( path.data(), keyEnd );
        path.remove_prefix( key.size() );
        root = &root->GetChild( key );
      }
      else if( *path.data() == '[' )
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
      TAC_ASSERT( oldPath != path );
    }
    TAC_ASSERT( root != &mJson );
    return root;
  }

  StringView SettingsGetString( StringView path, StringView fallback, Json* root )
  {
    return SettingsGetValue( path, &Json( fallback ), root )->mString;
  }

  void       SettingsSetString( StringView path, StringView setValue, Json* root )
  {
    SettingsSetValue( path, root, setValue );
  }

  JsonNumber SettingsGetNumber( StringView path, JsonNumber fallback, Json* root )
  {
    return SettingsGetValue( path, &Json( fallback ), root )->mNumber;
  }

  void       SettingsSetNumber( StringView path, JsonNumber setValue, Json* root )
  {
    SettingsSetValue( path, root, setValue );
  }

  bool       SettingsGetBool( StringView path, bool fallback, Json* root )
  {

    return SettingsGetValue( path, &Json( fallback ), root )->mBoolean;
  }

  void       SettingsSetBool( StringView path, bool setValue, Json* root )
  {
    SettingsSetValue( path, root, setValue );
  }

}
