#include "tac_level_editor_prefab.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/string/tac_string_util.h"

namespace Tac
{
  const char* refFrameVecNamePosition{ "mPos" };
  const char* refFrameVecNameForward{ "mForwards" };
  const char* refFrameVecNameRight{ "mRight" };
  const char* refFrameVecNameUp{ "mUp" };

  // this would be saved as a .map file in cod engine
  struct Prefab
  {
    //                Entities which are instances of this prefab
    Vector< Entity* > mEntities;

    //                Filepath of the serialized prefab
    AssetPathString   mAssetPath;
  };

  static Vector< Prefab* >  mPrefabs;
  static const char* prefabSettingsPath{ "prefabs" };

  static void         PrefabUpdateOpenedInEditor( SettingsNode settingsNode )
  {
    Vector< AssetPathString > documentPaths;

    for( Prefab* prefab : mPrefabs )
      for( Entity* entity : prefab->mEntities )
        if( !entity->mParent )
          documentPaths.push_back( prefab->mAssetPath );

    Json children;
    for( const AssetPathString& documentPath : documentPaths )
      *children.AddChild() = Json( documentPath.c_str() );

    settingsNode.GetChild( prefabSettingsPath ).SetValue( children );
  }

  static SettingsNode GetPrefabCameraNode( SettingsNode settingsNode, Prefab* prefab )
  {
    if( prefab->mAssetPath.empty() )
      return {};

    Span< SettingsNode > nodes{
      settingsNode.GetChild( "prefabCameraRefFrames" ).GetChildrenArray() };

    for( SettingsNode node : nodes )
      if( node.GetChild( "path" ).GetValue() == prefab->mAssetPath )
        return node;

    return {};
  }

  static void         PrefabLoadCameraVec( SettingsNode settingsNode,
                                           Prefab* prefab,
                                           StringView refFrameVecName,
                                           v3& refFrameVec )
  {
    SettingsNode node{ GetPrefabCameraNode( settingsNode, prefab ) };
    if( !node.IsValid() )
      return;

    for( int iAxis{}; iAxis < 3; ++iAxis )
      refFrameVec[ iAxis ] = ( float )node
        .GetChild( String() + refFrameVecName + "." + StringView( "xyz" + iAxis, 1 ) )
        .GetValueWithFallback( refFrameVec[ iAxis ] ).mNumber;
  }

  static void         PrefabSaveCameraVec( SettingsNode settingsNode,
                                           Prefab* prefab,
                                           StringView refFrameVecName,
                                           v3 refFrameVec )
  {
    SettingsNode node{ GetPrefabCameraNode( settingsNode, prefab ) };
    if( !node.IsValid() )
      return;

    for( int iAxis{}; iAxis < 3; ++iAxis )
    {
      node
        .GetChild( String() + refFrameVecName + "." + StringView( "xyz" + iAxis, 1 ) )
        .SetValue( refFrameVec[ iAxis ] );
    }

  }


  static void         PrefabLoadCamera( SettingsNode settingsNode, Prefab* prefab, Camera* camera )
  {
    if( !camera )
      return;

    PrefabLoadCameraVec( settingsNode, prefab, refFrameVecNamePosition, camera->mPos );
    PrefabLoadCameraVec( settingsNode, prefab, refFrameVecNameForward, camera->mForwards );
    PrefabLoadCameraVec( settingsNode, prefab, refFrameVecNameRight, camera->mRight );
    PrefabLoadCameraVec( settingsNode, prefab, refFrameVecNameUp, camera->mUp );
  }


  static void         AllocateEntityUUIDsRecursively( EntityUUIDCounter* entityUUIDCounter,
                                                      Entity* entityParent )
  {
    entityParent->mEntityUUID = entityUUIDCounter->AllocateNewUUID();
    for( Entity* entityChild : entityParent->mChildren )
      AllocateEntityUUIDsRecursively( entityUUIDCounter, entityChild );
  }


  static Prefab* PrefabFind( Entity* entity )
  {
    for( Prefab* prefab : mPrefabs )
    {
      if( Contains( prefab->mEntities, entity ) )
      {
        return prefab;
      }
    }
    return nullptr;
  }

  static void PrefabSaveEntity( Entity* entity, Errors& errors )
  {
    // Why?
    if( entity->mParent )
      return;

    Prefab* prefab{ PrefabFind( entity ) };
    if( !prefab )
    {
      prefab = TAC_NEW Prefab;
      prefab->mEntities = { entity };
      mPrefabs.push_back( prefab );
    }


    // Get document paths for prefabs missing them
    if( prefab->mAssetPath.empty() )
    {
      FileSys::Path suggestedFilename{ entity->mName + ".prefab" };
      const OS::SaveParams saveParams
      {
        .mSuggestedFilename { &suggestedFilename },
      };

      TAC_CALL( const FileSys::Path savePath{ OS::OSSaveDialog( saveParams, errors ) } );

      TAC_CALL( const AssetPathStringView assetPath{ ModifyPathRelative( savePath, errors ) } );

      prefab->mAssetPath = assetPath;
    }

    const Json entityJson{ entity->Save() };
    const String prefabJsonString{ entityJson.Stringify() };
    const void* bytes{ prefabJsonString.data() };
    const int byteCount{ prefabJsonString.size() };
    const FileSys::Path fsPath( prefab->mAssetPath );
    TAC_CALL( FileSys::SaveToFile( fsPath, bytes, byteCount, errors ) );
  }


  static void         PrefabRemoveEntityRecursivelyAux( Entity* entity )
  {
    int prefabCount{ mPrefabs.size() };
    for( int iPrefab{  }; iPrefab < prefabCount; ++iPrefab )
    {
      Prefab* prefab{ mPrefabs[ iPrefab ] };
      bool removedEntityFromPrefab{ false };
      int prefabEntityCount{ prefab->mEntities.size() };
      for( int iPrefabEntity{  }; iPrefabEntity < prefabEntityCount; ++iPrefabEntity )
      {
        if( prefab->mEntities[ iPrefabEntity ] == entity )
        {
          prefab->mEntities[ iPrefabEntity ] = prefab->mEntities[ prefabEntityCount - 1 ];
          prefab->mEntities.pop_back();
          if( prefab->mEntities.empty() )
          {
            mPrefabs[ iPrefab ] = mPrefabs[ prefabCount - 1 ];
            mPrefabs.pop_back();
          }

          removedEntityFromPrefab = true;
          break;
        }
      }

      if( removedEntityFromPrefab )
        break;
    }
    for( Entity* child : entity->mChildren )
      PrefabRemoveEntityRecursivelyAux( child );

  }


  static void         PrefabImGui( Prefab* prefab )
  {
    if( !ImGuiCollapsingHeader( prefab->mAssetPath ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;
    for( Entity* entity : prefab->mEntities )
    {
      if( ImGuiButton( String()
                       + "select entity of uuid "
                       + ToString( ( UUID )entity->mEntityUUID ) ) )
      {
        gCreation.mSelectedEntities.Select( entity );
      }
    }
  }
}

void                Tac::PrefabLoadAtPath( SettingsNode settingsNode,
                                           EntityUUIDCounter* entityUUIDCounter,
                                           World* world,
                                           Camera* camera,
                                           const AssetPathStringView& prefabPath,
                                           Errors& errors )
{
  TAC_CALL( const String memory{ LoadAssetPath( prefabPath, errors ) } );

  Json prefabJson;
  prefabJson.Parse( memory.data(), memory.size(), errors );

  Entity* entity{ world->SpawnEntity( NullEntityUUID ) };
  entity->Load( prefabJson );

  AllocateEntityUUIDsRecursively( entityUUIDCounter, entity );

  Prefab* prefab{ TAC_NEW Prefab
  {
    .mEntities  { entity },
    .mAssetPath { prefabPath },
  } };

  mPrefabs.push_back( prefab );

  PrefabLoadCamera( settingsNode, prefab, camera );

  PrefabUpdateOpenedInEditor( settingsNode);
}




void                Tac::PrefabLoad( SettingsNode settingsNode,
                                     EntityUUIDCounter* entityUUIDCounter,
                                World* world,
                                Camera* camera,
                                Errors& errors )
{
  TAC_UNUSED_PARAMETER( errors );

  Vector< String > paths;
  for( SettingsNode child : settingsNode.GetChild( prefabSettingsPath ).GetChildrenArray() )
    paths.push_back( child.GetValue().mString );


  for( String path : paths )
  {
    TAC_CALL( PrefabLoadAtPath( settingsNode, entityUUIDCounter, world, camera, path, errors ) );
  }
}



void                Tac::PrefabSave( World* world, Errors& errors )
{
  for( Entity* entity : world->mEntities )
    PrefabSaveEntity( entity, errors );
}

void                Tac::PrefabSaveCamera( SettingsNode settingsNode, Camera* camera )
{
  for( Prefab* prefab : mPrefabs )
  {
    PrefabSaveCameraVec( settingsNode, prefab, refFrameVecNamePosition, camera->mPos );
    PrefabSaveCameraVec( settingsNode, prefab, refFrameVecNameForward, camera->mForwards );
    PrefabSaveCameraVec( settingsNode, prefab, refFrameVecNameRight, camera->mRight );
    PrefabSaveCameraVec( settingsNode, prefab, refFrameVecNameUp, camera->mUp );
  }
}


void                Tac::PrefabRemoveEntityRecursively( SettingsNode settingsNode ,
                                                        Entity* entity )
{
  PrefabRemoveEntityRecursivelyAux( entity );
  PrefabUpdateOpenedInEditor( settingsNode );
}


// What if one of its ancestors is a prefab?
Tac::AssetPathStringView         Tac::PrefabGetOrNull( Entity* entity )
{
  for( Prefab* prefab : mPrefabs )
    if( Contains( prefab->mEntities, entity ) )
      return prefab->mAssetPath;
  return {};
}

Tac::AssetPathStringView          Tac::PrefabGetLoaded()
{
  for( Prefab* prefab : mPrefabs )
    for( Entity* entity : prefab->mEntities )
      if( !entity->mParent )
        return prefab->mAssetPath;
  return "";
}


void                Tac::PrefabImGui()
{
  if( mPrefabs.empty() )
  {
    ImGuiText( "no prefabs" );
    return;
  }

  if( !ImGuiCollapsingHeader( "Prefabs" ) )
    return;

  TAC_IMGUI_INDENT_BLOCK;
  for( Prefab* prefab : mPrefabs )
  {
    PrefabImGui( prefab );
  }
}

