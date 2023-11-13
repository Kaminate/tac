#include "src/level_editor/tac_level_editor_prefab.h" // self-include

#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/core/tac_algorithm.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/string/tac_string.h"
#include "src/common/string/tac_string_util.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/system/tac_os.h"
#include "src/level_editor/tac_level_editor.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"

import std; // #include <iostream>

namespace Tac
{
  const char* refFrameVecNamePosition = "mPos";
  const char* refFrameVecNameForward  = "mForwards";
  const char* refFrameVecNameRight    = "mRight";
  const char* refFrameVecNameUp       = "mUp";

  // this would be saved as a .map file in cod engine
  struct Prefab
  {
    //                Entities which are instances of this prefab
    Vector< Entity* > mEntities;

    //                Filepath of the serialized prefab
    AssetPathString   mAssetPath;
  };

  static Vector< Prefab* >  mPrefabs;
  static const char*        prefabSettingsPath = "prefabs";

  static void         PrefabUpdateOpenedInEditor()
  {
    Vector< AssetPathString > documentPaths;

    for( Prefab* prefab : mPrefabs )
      for( Entity* entity : prefab->mEntities )
        if( !entity->mParent )
          documentPaths.push_back( prefab->mAssetPath );

    Json* prefabJson = SettingsGetJson( prefabSettingsPath );
    prefabJson->Clear();
    for( const AssetPathString& documentPath : documentPaths )
      *prefabJson->AddChild() = Json( documentPath.c_str() );

    SettingsSave();
  }


  static void         PrefabLoadCameraVec( Prefab* prefab, StringView refFrameVecName, v3& refFrameVec )
  {
    if( prefab->mAssetPath.empty() )
      return;

    for( int iAxis = 0; iAxis < 3; ++iAxis )
    {
      //const StringView axisName = "xyz"[ iAxis ];
      Errors ignored;
      const JsonNumber defaultValue = refFrameVec[ iAxis ];

      Json* refFramesJson = SettingsGetJson( { "prefabCameraRefFrames" } );
      Json* refFrameJson = SettingsGetChildByKeyValuePair( "path", Json( prefab->mAssetPath ), refFramesJson );
      const JsonNumber axisValue = SettingsGetNumber( Join( { refFrameVecName, String( 1, "xyz"[ iAxis ] ) }, "." ),
                                                      defaultValue,
                                                      refFrameJson );
      refFrameVec[ iAxis ] = ( float )axisValue;
    }
  }

  static void         PrefabLoadCamera( Prefab* prefab, Camera* camera )
  {
    if( !camera )
      return;
    PrefabLoadCameraVec( prefab, refFrameVecNamePosition, camera->mPos );
    PrefabLoadCameraVec( prefab, refFrameVecNameForward, camera->mForwards );
    PrefabLoadCameraVec( prefab, refFrameVecNameRight, camera->mRight );
    PrefabLoadCameraVec( prefab, refFrameVecNameUp, camera->mUp );
  }

  static void         PrefabSaveCameraVec( Prefab* prefab, StringView refFrameVecName, v3 refFrameVec )
  {
    if( prefab->mAssetPath.empty() )
      return;

    for( int iAxis = 0; iAxis < 3; ++iAxis )
    {
      Json* refFramesJson = SettingsGetJson( { "prefabCameraRefFrames" } );
      Json* refFrameJson = SettingsGetChildByKeyValuePair( "path", Json( prefab->mAssetPath ), refFramesJson );
      SettingsSetNumber( Join( { refFrameVecName, String( 1, "xyz"[ iAxis ] ) }, "." ),
                         refFrameVec[ iAxis ],
                         refFrameJson );
    }

  }

  static void         AllocateEntityUUIDsRecursively( EntityUUIDCounter* entityUUIDCounter,
                                                      Entity* entityParent )
  {
    entityParent->mEntityUUID = entityUUIDCounter->AllocateNewUUID();
    for( Entity* entityChild : entityParent->mChildren )
      AllocateEntityUUIDsRecursively( entityUUIDCounter, entityChild );
  }

  AssetPathStringView          PrefabGetLoaded()
  {
    for( Prefab* prefab : mPrefabs )
      for( Entity* entity : prefab->mEntities )
        if( !entity->mParent )
          return prefab->mAssetPath;
    return "";
  }

  void                PrefabLoadAtPath( EntityUUIDCounter* entityUUIDCounter,
                                        World* world,
                                        Camera* camera,
                                        const AssetPathStringView& prefabPath,
                                        Errors& errors )
  {
    //ModifyPathRelative( prefabPath );
    const String memory = Filesystem::LoadAssetPath( prefabPath, errors );
    TAC_HANDLE_ERROR( errors );

    Json prefabJson;
    prefabJson.Parse( memory.data(), memory.size(), errors );


    Entity* entity = world->SpawnEntity( NullEntityUUID );
    entity->Load( prefabJson );

    AllocateEntityUUIDsRecursively( entityUUIDCounter, entity );


    auto prefab = TAC_NEW Prefab
    {
      //prefab->mDocumentPath = prefabPath;
      .mEntities = { entity },
      .mAssetPath = prefabPath,
    };

    mPrefabs.push_back( prefab );

    PrefabLoadCamera( prefab, camera );

    PrefabUpdateOpenedInEditor();
  }

  void                PrefabLoad( EntityUUIDCounter* entityUUIDCounter,
                                  World* world,
                                  Camera* camera,
                                  Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Json* prefabs = SettingsGetJson( prefabSettingsPath );
    Vector< String > paths;
    for( Json* child : prefabs->mArrayElements )
      paths.push_back( child->mString );
    for( String path : paths )
    {
      PrefabLoadAtPath( entityUUIDCounter, world, camera, path, errors );
      TAC_HANDLE_ERROR( errors );
    }
  }

  static Prefab*      PrefabFind( Entity* entity )
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

    Prefab* prefab = PrefabFind( entity );
    if( !prefab )
    {
      prefab = TAC_NEW Prefab;
      prefab->mEntities = { entity };
      mPrefabs.push_back( prefab );
    }


    // Get document paths for prefabs missing them
    if( prefab->mAssetPath.empty() )
    {
      const Filesystem::Path suggestedName = entity->mName + ".prefab";
      
      Filesystem::Path savePath = OS::OSSaveDialog( suggestedName, errors );
      TAC_HANDLE_ERROR( errors );

      AssetPathStringView assetPath = ModifyPathRelative( savePath, errors );
      TAC_HANDLE_ERROR( errors );

      prefab->mAssetPath = assetPath;
    }

    Json entityJson;
    entity->Save( entityJson );

    String prefabJsonString = entityJson.Stringify();
    const void* bytes = prefabJsonString.data();
    const int byteCount = prefabJsonString.size();
    const Filesystem::Path fsPath( prefab->mAssetPath );
    Filesystem::SaveToFile( fsPath, bytes, byteCount, errors );
    TAC_HANDLE_ERROR( errors );
  }

  void                PrefabSave( World* world, Errors& errors )
  {
    for( Entity* entity : world->mEntities )
    {
      PrefabSaveEntity( entity, errors );
    }

  }

  void                PrefabSaveCamera( Camera* camera )
  {
    for( Prefab* prefab : mPrefabs )
    {
      PrefabSaveCameraVec( prefab, refFrameVecNamePosition, camera->mPos );
      PrefabSaveCameraVec( prefab, refFrameVecNameForward, camera->mForwards );
      PrefabSaveCameraVec( prefab, refFrameVecNameRight, camera->mRight );
      PrefabSaveCameraVec( prefab, refFrameVecNameUp, camera->mUp );
    }
  }

  static void         PrefabRemoveEntityRecursivelyAux( Entity* entity )
  {
    int prefabCount = mPrefabs.size();
    for( int iPrefab = 0; iPrefab < prefabCount; ++iPrefab )
    {
      Prefab* prefab = mPrefabs[ iPrefab ];
      bool removedEntityFromPrefab = false;
      int prefabEntityCount = prefab->mEntities.size();
      for( int iPrefabEntity = 0; iPrefabEntity < prefabEntityCount; ++iPrefabEntity )
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

  void                PrefabRemoveEntityRecursively( Entity* entity )
  {
    PrefabRemoveEntityRecursivelyAux( entity );
    PrefabUpdateOpenedInEditor();
  }

  // What if one of its ancestors is a prefab?
  AssetPathStringView         PrefabGetOrNull( Entity* entity )
  {
    for( Prefab* prefab : mPrefabs )
      if( Contains( prefab->mEntities, entity ) )
        return prefab->mAssetPath;
    return {};
  }

  static void         PrefabImGui( Prefab* prefab )
  {
    if( !ImGuiCollapsingHeader( prefab->mAssetPath ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;
    for( Entity* entity : prefab->mEntities )
    {
      const char* buttonText = FrameMemoryPrintf( "select entity of uuid %i", ( int )entity->mEntityUUID );
      if( ImGuiButton( buttonText ) )
        gCreation.mSelectedEntities.Select( entity );
    }
  }
  void                PrefabImGui()
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
}

