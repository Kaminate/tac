#include "src/common/containers/tacVector.h"
#include "src/common/tacCamera.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacOS.h"
#include "src/common/tacSettings.h"
#include "src/common/tacUtility.h"
#include "src/common/string/tacString.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/creation/tacCreationPrefab.h"
#include "src/creation/tacCreation.h"
#include "src/space/tacentity.h"
#include "src/space/tacworld.h"

#include <iostream>

namespace Tac
{
  const char* refFrameVecNamePosition = "mPos";
  const char* refFrameVecNameForward = "mForwards";
  const char* refFrameVecNameRight = "mRight";
  const char* refFrameVecNameUp = "mUp";

  // this would be saved as a .map file in cod engine
  struct Prefab
  {
    //                Entities which are instances of this prefab
    Vector< Entity* > mEntities;

    //                Filepath of the serialized prefab
    Tac::String       mDocumentPath;
  };

  static Vector< Prefab* >  mPrefabs;
  static const char*             prefabSettingsPath = "prefabs";


  static void         PrefabLoadCameraVec( Prefab* prefab, StringView refFrameVecName, v3& refFrameVec )
  {
    if( prefab->mDocumentPath.empty() )
      return;
    for( int iAxis = 0; iAxis < 3; ++iAxis )
    {
      //const StringView axisName = "xyz"[ iAxis ];
      Errors ignored;
      const JsonNumber defaultValue = refFrameVec[ iAxis ];

      Json* refFramesJson = SettingsGetJson( { "prefabCameraRefFrames" } );
      Json* refFrameJson = SettingsGetChildByKeyValuePair( "path", Json( prefab->mDocumentPath ), refFramesJson );
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
    if( prefab->mDocumentPath.empty() )
      return;
    for( int iAxis = 0; iAxis < 3; ++iAxis )
    {
      Json* refFramesJson = SettingsGetJson( { "prefabCameraRefFrames" } );
      Json* refFrameJson = SettingsGetChildByKeyValuePair( "path", Json( prefab->mDocumentPath ), refFramesJson );
      SettingsSetNumber( Join( { refFrameVecName, String( 1, "xyz"[ iAxis ] ) }, "." ),
                         refFrameVec[ iAxis ],
                         refFrameJson );
    }

  }

  void                PrefabLoadAtPath( World* world, Camera* camera, String prefabPath, Errors& errors )
  {
    ModifyPathRelative( prefabPath );
    TemporaryMemory memory = TemporaryMemoryFromFile( prefabPath, errors );
    TAC_HANDLE_ERROR( errors );

    Json prefabJson;
    prefabJson.Parse( memory.data(), memory.size(), errors );

    Entity* entity = world->SpawnEntity( NullEntityUUID );
    entity->Load( prefabJson );

    auto prefab = TAC_NEW Prefab;
    prefab->mDocumentPath = prefabPath;
    prefab->mEntities = { entity };
    mPrefabs.push_back( prefab );

    PrefabLoadCamera( prefab, camera );
  }

  void                PrefabLoad( World* world, Camera* camera, Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Json* prefabs = SettingsGetJson( prefabSettingsPath );
    for( Json* child : prefabs->mArrayElements )
    {
      PrefabLoadAtPath( world, camera, child->mString, errors );
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

  void                PrefabSave( World* world )
  {
    for( Entity* entity : world->mEntities )
    {
      if( entity->mParent )
        continue;

      Prefab* prefab = PrefabFind( entity );
      if( !prefab )
      {
        prefab = TAC_NEW Prefab;
        prefab->mEntities = { entity };
        mPrefabs.push_back( prefab );
      }


      // Get document paths for prefabs missing them
      if( prefab->mDocumentPath.empty() )
      {
        String savePath;
        const String suggestedName = entity->mName + ".prefab";
        Errors saveDialogErrors;
        OSSaveDialog( savePath, suggestedName, saveDialogErrors );
        if( saveDialogErrors )
        {
          // todo: log it, user feedback
          std::cout << saveDialogErrors.ToString().c_str() << std::endl;
          continue;
        }

        ModifyPathRelative( savePath );

        prefab->mDocumentPath = savePath;
      }

      //Entity* entity = prefab->mEntity;

      Json entityJson;
      entity->Save( entityJson );

      String prefabJsonString = entityJson.Stringify();
      Errors saveToFileErrors;
      void* bytes = prefabJsonString.data();
      int byteCount = prefabJsonString.size();
      OSSaveToFile( prefab->mDocumentPath, bytes, byteCount, saveToFileErrors );
      if( saveToFileErrors )
      {
        // todo: log it, user feedback
        std::cout << saveToFileErrors.ToString().c_str() << std::endl;
        continue;
      }
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

  void                PrefabRemoveEntityRecursively( Entity* entity )
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
      PrefabRemoveEntityRecursively( child );
  }

  // What if one of its ancestors is a prefab?
  const char*         PrefabGetOrNull( Entity* entity )
  {
    for( Prefab* prefab : mPrefabs )
      if( Contains( prefab->mEntities, entity ) )
        return prefab->mDocumentPath;
    return nullptr;
  }
}

