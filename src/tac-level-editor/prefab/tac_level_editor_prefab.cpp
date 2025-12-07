#include "tac_level_editor_prefab.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/selection/tac_level_editor_entity_selection.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/string/tac_string_util.h"

namespace Tac
{
  // this would be saved as a .map file in cod engine
  struct Prefab
  {
    Vector< Entity* > mEntities;  // Entities which are instances of this prefab
    AssetPathString   mAssetPath; // Filepath of the serialized prefab
  };

  struct CameraSaveHelper
  {
    const char* mJsonName {};
    int         mOffset   {};
  };

  static const char*            kRefFrameVecNamePosition { "mPos" };
  static const char*            kRefFrameVecNameForward  { "mForwards" };
  static const char*            kRefFrameVecNameRight    { "mRight" };
  static const char*            kRefFrameVecNameUp       { "mUp" };
  static const CameraSaveHelper kCameraSaveHelpers[]
  {
    CameraSaveHelper{.mJsonName{kRefFrameVecNamePosition}, .mOffset{TAC_OFFSET_OF(Camera, mPos)}},
    CameraSaveHelper{.mJsonName{kRefFrameVecNameForward}, .mOffset{TAC_OFFSET_OF(Camera, mForwards)}},
    CameraSaveHelper{.mJsonName{kRefFrameVecNameRight}, .mOffset{TAC_OFFSET_OF(Camera, mRight)}},
    CameraSaveHelper{.mJsonName{kRefFrameVecNameUp}, .mOffset{TAC_OFFSET_OF(Camera, mUp)}},
  };


  static const char*        kPrefabSettingsPath      { "prefabs" };
  static Vector< Prefab* >  sPrefabs                 {};

  static void PrefabUpdateOpenedInEditor()
  {
    Vector< AssetPathString > documentPaths;

    for( Prefab* prefab : sPrefabs )
      for( Entity* entity : prefab->mEntities )
        if( !entity->mParent )
          documentPaths.push_back( prefab->mAssetPath );

    Json children;
    for( const AssetPathString& documentPath : documentPaths )
      *children.AddChild() = Json( documentPath.c_str() );
    SettingsNode settingsNode{ Shell::sShellSettings };
    settingsNode.GetChild( kPrefabSettingsPath ).SetValue( children );
  }

  static auto GetPrefabCameraNode( SettingsNode settingsNode, const AssetPathStringView prefabPath ) -> SettingsNode
  {
    if( prefabPath.empty() )
      return {}; // when would this ever happen?

    Span< SettingsNode > nodes{
      settingsNode.GetChild( "prefabCameraRefFrames" ).GetChildrenArray() };

    for( SettingsNode node : nodes )
      if( node.GetChild( "path" ).GetValue() == prefabPath )
        return node;

    return {};
  }


  static auto PrefabFind( Entity* entity ) -> Prefab*
  {
    for( Prefab* prefab : sPrefabs )
    {
      if( Contains( prefab->mEntities, entity ) )
      {
        return prefab;
      }
    }
    return nullptr;
  }

  static bool PrefabSaveEntity( Entity* entity, Errors& errors )
  {
    // Why?
    if( entity->mParent )
      return false;

    Prefab* prefab{ PrefabFind( entity ) };
    if( !prefab )
    {
      prefab = TAC_NEW Prefab;
      prefab->mEntities = { entity };
      sPrefabs.push_back( prefab );
    }

    // Get document paths for prefabs missing them
    if( prefab->mAssetPath.empty() )
    {
      TAC_CALL_RET( prefab->mAssetPath = AssetSaveDialog(
        AssetSaveDialogParams{ .mSuggestedFilename {entity->mName + ".prefab"} },
        errors ) );
    }

    const Json entityJson{ entity->Save() };
    const String prefabJsonString{ entityJson.Stringify() };
    const void* bytes{ prefabJsonString.data() };
    const int byteCount{ prefabJsonString.size() };
    TAC_CALL_RET( SaveToFile( prefab->mAssetPath, bytes, byteCount, errors ) );
    return true;
  }


  static void PrefabRemoveEntityRecursivelyAux( Entity* entity )
  {
    int prefabCount{ sPrefabs.size() };
    for( int iPrefab{  }; iPrefab < prefabCount; ++iPrefab )
    {
      Prefab* prefab{ sPrefabs[ iPrefab ] };
      bool removedEntityFromPrefab{};
      int prefabEntityCount{ prefab->mEntities.size() };
      for( int iPrefabEntity{  }; iPrefabEntity < prefabEntityCount; ++iPrefabEntity )
      {
        if( prefab->mEntities[ iPrefabEntity ] == entity )
        {
          prefab->mEntities[ iPrefabEntity ] = prefab->mEntities[ prefabEntityCount - 1 ];
          prefab->mEntities.pop_back();
          if( prefab->mEntities.empty() )
          {
            sPrefabs[ iPrefab ] = sPrefabs[ prefabCount - 1 ];
            sPrefabs.pop_back();
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


  static void PrefabImGui( Prefab* prefab )
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
        SelectedEntities::Select( entity );
      }
    }
  }
}

void Tac::PrefabLoadAtPath( const AssetPathStringView prefabPath, Errors& errors )
{
  World* world{ Creation::GetWorld() };
  EntityUUIDCounter* entityUUIDCounter{ Creation::GetEntityUUIDCounter() };
  TAC_CALL( const String memory{ LoadAssetPath( prefabPath, errors ) } );
  TAC_CALL( const Json prefabJson{ Json::Parse( memory, errors ) } );

  if( world->mEntities.empty() )
    if( Camera * camera{ Creation::GetEditorCamera() } )
      if( SettingsNode node{ GetPrefabCameraNode( Shell::sShellSettings, prefabPath ) };
          node.IsValid() )
        for( int iAxis{}; iAxis < 3; ++iAxis )
          for( CameraSaveHelper helper : kCameraSaveHelpers )
            if( v3& v{ *( v3* )( ( u8* )camera + helper.mOffset ) }; true )
              v[ iAxis ] = ( float )node
              .GetChild( String() + helper.mJsonName + "." + StringView( "xyz" + iAxis, 1 ) )
              .GetValueWithFallback( v[ iAxis ] ).mNumber;

  Entity* entity{ world->SpawnEntity( entityUUIDCounter->AllocateNewUUID() ) };
  entity->Load( *entityUUIDCounter, prefabJson );
  auto prefab{ TAC_NEW Prefab
  {
    .mEntities  { entity },
    .mAssetPath { prefabPath },
  } };
  sPrefabs.push_back( prefab );
  PrefabUpdateOpenedInEditor();
}

void Tac::PrefabLoad(  Errors& errors )
{
  SettingsNode settingsNode{ Shell::sShellSettings };
  //                  TODO: when prefabs spawn, they need to have a entity uuid.
  //                        this can only be done by a serverdata.
  //
  //                        So this function should also have a serverdata parameter
  TAC_UNUSED_PARAMETER( errors );

  Vector< String > paths;
  for( SettingsNode child : settingsNode.GetChild( kPrefabSettingsPath ).GetChildrenArray() )
    paths.push_back( child.GetValue().mString );

  for( StringView path : paths )
  {
    TAC_CALL( PrefabLoadAtPath(  path, errors ) );
  }
}

bool Tac::PrefabSave( World* world, Errors& errors )
{
  bool result {};
  for( Entity* entity : world->mEntities )
    result |= PrefabSaveEntity( entity, errors );

  return result;
}

void Tac::PrefabSaveCamera( const Camera* camera, const AssetPathStringView prefabPath )
{
  SettingsNode settingsNode{ Shell::sShellSettings };
  for( Prefab* prefab : sPrefabs )
    if( ( AssetPathStringView )prefab->mAssetPath == prefabPath )
      if( SettingsNode node{ GetPrefabCameraNode( settingsNode, prefabPath ) };
          node.IsValid() )
        for( CameraSaveHelper helper : kCameraSaveHelpers )
          if( const v3& v{ *( const v3* )( ( const u8* )camera + helper.mOffset ) }; true )
            for( int iAxis{}; iAxis < 3; ++iAxis )
              node
              .GetChild( String() + helper.mJsonName + "." + StringView( "xyz" + iAxis, 1 ) )
              .SetValue( v[ iAxis ] );
}

void Tac::PrefabRemoveEntityRecursively(  Entity* entity )
{
  PrefabRemoveEntityRecursivelyAux( entity );
  PrefabUpdateOpenedInEditor( );
}

// What if one of its ancestors is a prefab?
auto Tac::PrefabGetOrNull( Entity* entity ) -> Tac::AssetPathStringView
{
  for( Prefab* prefab : sPrefabs )
    if( Contains( prefab->mEntities, entity ) )
      return prefab->mAssetPath;
  return {};
}

auto Tac::PrefabGetLoaded() -> Tac::AssetPathStringView
{
  for( Prefab* prefab : sPrefabs )
    for( Entity* entity : prefab->mEntities )
      if( !entity->mParent )
        return prefab->mAssetPath;
  return "";
}


void Tac::PrefabImGui()
{
  if( sPrefabs.empty() )
  {
    ImGuiText( "no prefabs" );
    return;
  }

  if( !ImGuiCollapsingHeader( "Prefabs" ) )
    return;

  TAC_IMGUI_INDENT_BLOCK;
  for( Prefab* prefab : sPrefabs )
  {
    PrefabImGui( prefab );
  }
}

