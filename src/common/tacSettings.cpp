#include "src/common/tacSettings.h"
#include "src/common/tacUtility.h"
#include "src/common/tacMemory.h"
#include "src/common/tacOS.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacTemporaryMemory.h"
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

  //static Json* SettingsStepJsonObject( StringView* path, Json* root )
  //{
  //  //path->remove_prefix( path->starts_with( "." ) || path->starts_with( "[" ) ? 1 : 0 );

  //  StringView key;
  //  const int iKeyEnd = path->find_first_of( ".]" );
  //  if( iKeyEnd == StringView::npos )
  //  {
  //    key = *path;
  //    *path = StringView();
  //  }
  //  else
  //  {
  //    key = StringView( path->data(), iKeyEnd );
  //    path->remove_prefix( key.size() );
  //    //path->remove_prefix( path->starts_with( "]" ) ? 1 : 0 );
  //    path->remove_prefix( 1 );
  //  }
  //  root = &root->GetChild( key );
  //  return root;
  //}

  static void SettingsSetValue( StringView path, Json* root, Json setValue )
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

  void SettingsInit( Errors& errors )
  {
    if( !FileExist( SettingsGetSavePath() ) )
    {
      SettingsSave( errors );
      TAC_HANDLE_ERROR( errors );
      return;
    }
    auto temporaryMemory = TemporaryMemoryFromFile( SettingsGetSavePath(), errors );
    mJson.Parse( temporaryMemory.data(), ( int )temporaryMemory.size(), errors );
    //mJson.mType = JsonType::Object;
  }

  void SettingsSave( Errors& errors )
  {
    String str = mJson.Stringify();
    OS::SaveToFile( SettingsGetSavePath(), ( void* )str.data(), ( int )str.size(), errors );
    TAC_HANDLE_ERROR( errors );
  }

  //static Json* GetLeafNode( StringView path, Json* json )
  //{
  //	json = json ? json : &mJson;
  //	for( ;; )
  //	{
  //		const int iDot = path.find_first_of( "." );
  //		if( iDot != StringView::npos )
  //		{
  //			path = path.data() + iDot + 1;
  //		}
  //		break;
  //	}
  //	return nullptr;
  //}

  //StringView              SettingsGetString( StringView path, StringView defaultValue, Json* json )
  //{
  //	json = GetLeafNode( path, json );
  //	const bool changed = json->mString == 
  //	json->
  //}

  Json*                   SettingsGetJson( StringView path, Json* root )
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
        //root = SettingsStepJsonObject( &path, root );
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
        TAC_ASSERT( ( unsigned )iElement < ( unsigned )root->mElements.size() );
        root = root->mElements[ iElement ];
        path.remove_prefix( iCloseSquareBracket );
      }

      //if( root->mType == JsonType::Null )
      //{
        //if( IsAlpha( *path.data() ) ) 
        //  root->mType = JsonType::Object;
        //if( *path.data() >= '0' && *path.data() <= '9' ) 
        //  root->mType = JsonType::Array;
        //if( path.starts_with( "[" ) )
        //{
        //}
        //else if( path.starts_with( "[" ) )
        //{
        //  const int iCloseBracket = path.find_first_of( "]" );
        //  TAC_ASSERT( iCloseBracket != StringView::npos );
        //  bool numerKey = true;
        //  for( char c : StringView( path.data() + 1, path.data() + iCloseBracket ) )
        //    if( !( c >= '0' && c <= '9' ) )
        //      numerKey = false;
        //  root->mType = numerKey ? JsonType::Array : JsonType::Object;
        //}
    //}

    //if( root->mType == JsonType::Object )
    //  root = SettingsStepJsonObject( &path, root );
      TAC_ASSERT( oldRoot != root );
      TAC_ASSERT( oldPath != path );
    }
    TAC_ASSERT( root != &mJson );
    return root;
  }

  StringView              SettingsGetString( StringView path, StringView fallback, Json* root )
  {
    return SettingsGetValue( path, &Json( fallback ), root )->mString;
  }

  void                    SettingsSetString( StringView path, StringView setValue, Json* root )
  {
    SettingsSetValue( path, root, setValue );
  }

  JsonNumber              SettingsGetNumber( StringView path, JsonNumber fallback, Json* root )
  {
    return SettingsGetValue( path, &Json( fallback ), root )->mNumber;
  }

  void                    SettingsSetNumber( StringView path, JsonNumber setValue, Json* root )
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

  //StringView              SettingsGetString( Json* json, StringView value )
  //{
  //  TAC_ASSERT_UNIMPLEMENTED;
  //  return "";
  //}

  //StringView              SettingsSetString( Json* json, StringView value )
  //{
  //  TAC_ASSERT_UNIMPLEMENTED;
  //  return "";
  //}

  //JsonNumber              SettingsGetNumber( Json* json, JsonNumber value )
  //{
  //  TAC_ASSERT_UNIMPLEMENTED;
  //  return 0;
  //}

  //JsonNumber              SettingsSetNumber( Json* json, JsonNumber value )
  //{
  //  TAC_ASSERT_UNIMPLEMENTED;
  //  return 0;
  //}

  //bool                    SettingsGetBool( Json* json, bool value )
  //{
  //  TAC_ASSERT_UNIMPLEMENTED;
  //  return false;
  //}

  //bool                    SettingsSetBool( Json* json, bool value )
  //{
  //  TAC_ASSERT_UNIMPLEMENTED;

  //  return false;
  //}


  //void Settings::GetSetting( Json* settingTree,
  //                           const SettingPath& settingVariables,
  //                           int iPath,
  //                           Json** outputSetting,
  //                           const Json& defaultValue,
  //                           Errors& errors )
  //{
  //  if( !settingTree )
  //    settingTree = &mJson;
  //  const StringView settingVariable = settingVariables[ iPath ];
  //  Json& child = settingTree->operator[]( settingVariable );
  //  bool isLeaf = iPath == ( int )settingVariables.size() - 1;
  //  if( isLeaf )
  //  {
  //    if( child.mType == JsonType::Null )
  //    {
  //      child = defaultValue;
  //      SettingsSave( errors );
  //      if( errors )
  //      {
  //        String getSettingsErrors;
  //        getSettingsErrors += "Failed to save default setting ";
  //        getSettingsErrors += settingVariable;
  //        getSettingsErrors += defaultValue.Stringify();
  //        errors.mMessage += getSettingsErrors;
  //        return;
  //      }
  //    }
  //    *outputSetting = &child;
  //    return;
  //  }
  //  GetSetting( &child, settingVariables, iPath + 1, outputSetting, defaultValue, errors );
  //}

  //void Settings::SetSetting( Json* settingTree,
  //                           const SettingPath& paths,
  //                           int iPath,
  //                           const Json& value,
  //                           Errors& errors )
  //{
  //  if( !settingTree )
  //    settingTree = &mJson;
  //  const StringView settingVariable = paths[ iPath ];
  //  Json& child = settingTree->operator[]( settingVariable );
  //  bool isLeaf = iPath == ( int )paths.size() - 1;
  //  if( isLeaf )
  //  {
  //    child = value;
  //    SettingsSave( errors );
  //    if( errors )
  //    {
  //      String getSettingsErrors;
  //      getSettingsErrors += "Failed to save default setting ";
  //      getSettingsErrors += settingVariable;
  //      getSettingsErrors += value.Stringify();
  //      errors.mMessage += getSettingsErrors;
  //      return;
  //    }
  //    return;
  //  }
  //  SetSetting( &child, paths, iPath + 1, value, errors );
  //}

  //bool Settings::GetBool( Json* root,
  //                        const SettingPath& paths,
  //                        bool defaultValue,
  //                        Errors& errors )
  //{
  //  Json* setting;
  //  GetSetting( root, paths, 0, &setting, Json( defaultValue ), errors );
  //  return setting->mBoolean;
  //}

  //JsonNumber Settings::GetNumber( Json* root,
  //                                const SettingPath& paths,
  //                                JsonNumber defaultValue,
  //                                Errors& errors )
  //{
  //  Json* setting;
  //  GetSetting( root, paths, 0, &setting, Json( defaultValue ), errors );
  //  return setting->mNumber;
  //}

  //void Settings::SetNumber( Json* root,
  //                          const SettingPath& paths,
  //                          JsonNumber value,
  //                          Errors& errors )
  //{
  //  SetSetting( root, paths, 0, Json( value ), errors );
  //}

  //StringView Settings::GetString( Json* root,
  //                                const SettingPath& paths,
  //                                String defaultValue,
  //                                Errors& errors )
  //{
  //  Json* setting;
  //  GetSetting( root, paths, 0, &setting, Json( defaultValue ), errors );
  //  return setting->mString;
  //}

  //void Settings::SetString( Json* root,
  //                          const SettingPath& paths,
  //                          StringView value,
  //                          Errors& errors )
  //{
  //  SetSetting( root, paths, 0, value, errors );
  //}

  //Json* Settings::GetArray( Json* root,
  //                          const SettingPath& paths,
  //                          Json* defaultValue,
  //                          Errors& errors )
  //{
  //  Json emptyArray;
  //  emptyArray.mType = JsonType::Array;
  //  if( !defaultValue )
  //    defaultValue = &emptyArray;
  //  Json* setting;
  //  GetSetting( root, paths, 0, &setting, *defaultValue, errors );
  //  return setting;
  //}

  //Json* Settings::GetObject( Json* root,
  //                           const SettingPath& paths,
  //                           Json* defaultValue,
  //                           Errors& errors )
  //{
  //  Json emptyObject;
  //  emptyObject.mType = JsonType::Object;
  //  if( !defaultValue )
  //    defaultValue = &emptyObject;
  //  Json* setting;
  //  GetSetting( root, paths, 0, &setting, *defaultValue, errors );
  //  return setting;
  //}

}

