#include "tac_level_editor_cube_tool.h" // self-inc

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/physics/collider/tac_collider.h"
#include "tac-level-editor/windows/game/tac_level_editor_game_window.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/picking/tac_level_editor_mouse_picking.h"

namespace Tac
{

  struct PickingPreview
  {
    bool mIsValid;
    v3 mUnitNormal;
    v3 mPosition;
  };

  static float cubeSize{ 1 };
  static bool  sPlacingCubes{};
  static Plane sPlacingCubePlane {};

  static auto Snap( v3 previewOrigin ) -> v3
  {
    float snap{ cubeSize };
    for( float& x : previewOrigin )
    {
      x -= .5f;
      x = Round( x / snap ) * ( snap );
      //x = Floor( x ) + ( Round( ( x - Floor( x ) ) * ( 1.0f / snap ) ) * snap );
      x += .5f;
    }
    return previewOrigin;
  }

  static auto GetEntityPreviewPos( const Entity* pickedEntity ) -> PickingPreview
  {
    const Ray mouseRay_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
    const Model* model{ Model::GetModel( pickedEntity ) };
    if( !model )
      return {};

    Errors errors;
    const ModelAssetManager::Params getMeshParams
    {
      .mPath       { model->mModelPath },
      .mModelIndex { model->mModelIndex },
    };

    const Mesh* mesh{ ModelAssetManager::GetMesh( getMeshParams, errors ) };
    TAC_ASSERT( !errors );
    if( !mesh )
      return {};

    const MeshRaycast::Result raycastResult_worldspace{ mesh->mMeshRaycast.Raycast_worldspace( mouseRay_worldspace, pickedEntity->mWorldTransform ) };
    if( !raycastResult_worldspace.IsValid() )
      return {};

    const Triangle closest_modelspace{ mesh->mMeshRaycast.mTris[ raycastResult_worldspace.mTriIdx ] };
    const Triangle closest_worldspace{
      .mVertices{
        ( pickedEntity->mWorldTransform * v4( closest_modelspace.mVertices[ 0 ], 1 ) ).xyz(),
        ( pickedEntity->mWorldTransform * v4( closest_modelspace.mVertices[ 1 ], 1 ) ).xyz(),
        ( pickedEntity->mWorldTransform * v4( closest_modelspace.mVertices[ 2 ], 1 ) ).xyz(),
      },
    };
    const v3 e1_worldspace{ closest_worldspace.mVertices[ 1 ] - closest_worldspace.mVertices[ 0 ] };
    const v3 e2_worldspace{ closest_worldspace.mVertices[ 2 ] - closest_worldspace.mVertices[ 0 ] };
    return PickingPreview{
      .mIsValid    { true },
      .mUnitNormal { Normalize( Cross( e1_worldspace, e2_worldspace ) )  },
      .mPosition   { mouseRay_worldspace.mOrigin + mouseRay_worldspace.mDirection * raycastResult_worldspace.mT },
    };
  }

  static auto GetGroundPreviewPos() -> PickingPreview
  {
    Ray mouseRay_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
    Plane ground{ Plane::FromPosUnitNormal( {}, v3( 0, 1, 0 ) ) };
    RayPlane rayPlane{ mouseRay_worldspace, ground };
    if( !rayPlane )
      return {};
    v3 hitPoint_worldspace{ rayPlane.mT * mouseRay_worldspace.mDirection + mouseRay_worldspace.mOrigin };
    //v3 previewOrigin_unsnapped{ hitPoint + ground.UnitNormal() * cubeSize / 2 };
    //return Snap( previewOrigin_unsnapped );
    return PickingPreview{
      .mIsValid{ true },
      .mUnitNormal{ ground.UnitNormal() },
      .mPosition {hitPoint_worldspace},
    };
  }

  static auto RandomColor() -> v4
  {
    v3 color{};
    float t{ RandomFloat0To1() };
    for( int i{}; i < 3; ++i )
      color[ i ]
      = v3( 0.3f, 0.1f, 0.3f )[ i ]
      + v3( 0.2f, 0.2f, 0.2f )[ i ] * Cos( 6.283185f * (
        v3( 1.0f, 1.0f, 1.0f )[ i ] * t +
        v3( 0.1f, 0.2f, 0.2f )[ i ] ) );
    return v4( color, 1 );
  }

  static auto SpawnCube( v3 spawnPos )
  {
      // this should say spawn prefab instance

      Entity* entity{ Creation::CreateEntity() };
      entity->mRelativeSpace.mPosition = spawnPos;
      entity->mStatic = true;

      auto model{ ( Model* )entity->AddNewComponent( Model().GetEntry() ) };
      model->mModelPath = "assets/essential/models/box/Box.gltf";
      model->mModelIndex = 0;

      auto material{ ( Material* )entity->AddNewComponent( Material().GetEntry() ) };
      material->mColor = RandomColor();
      material->mShaderGraph = "assets/shader-graphs/gltf_pbr.tac.sg";
      material->mIsGlTF_PBR_MetallicRoughness = true;
      material->mPBR_Factor_Roughness = 1;

      auto collider{ (Collider*) entity->AddNewComponent( Collider().GetEntry() ) };
      //collider->SET SHAPE AS CUBE;

  }

  CubeTool CubeTool::sInstance;
  CubeTool::CubeTool()
  {
    mDisplayName = "Cube";
    mIcon = "assets/editor/tools/cube_tool.png";
  }
  void CubeTool::OnToolSelected()
  {
  }
  void CubeTool::Update()
  {
    if( sPlacingCubes )
      if( !AppWindowApi::IsHovered( CreationGameWindow::GetWindowHandle() ) ||
          !AppKeyboardApi::IsPressed( Key::MouseLeft ) )
        sPlacingCubes = false;

    if( !AppWindowApi::IsHovered( CreationGameWindow::GetWindowHandle() ) )
      return;

    const Ray mouseRay_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };

    const World* world{ Creation::GetWorld() };
    dynmc Debug3DDrawData* drawData{ world->mDebug3DDrawData };
    const Entity* pickedEntity{ CreationMousePicking::GetPickedEntity() };
    const PickingPreview pickingPreview{ pickedEntity ? GetEntityPreviewPos( pickedEntity ) : GetGroundPreviewPos() };
    if( !pickingPreview.mIsValid )
      return;

    const v3 spawnPos{ Snap( pickingPreview.mPosition + pickingPreview.mUnitNormal * cubeSize / 2 ) };
    drawData->DebugDraw3DOBB( spawnPos, v3(1,1,1) * cubeSize / 2, m3() );

    bool shouldSpawnCube{};
    shouldSpawnCube |= AppKeyboardApi::JustPressed( Key::MouseLeft );
    shouldSpawnCube |= sPlacingCubes
      && Abs( sPlacingCubePlane.SignedDistance( pickingPreview.mPosition ) ) < .01f
      && Dot( sPlacingCubePlane.UnitNormal(), pickingPreview.mUnitNormal ) > .99f;
    if( shouldSpawnCube )
    {
      sPlacingCubePlane = Plane::FromPosUnitNormal( pickingPreview.mPosition, pickingPreview.mUnitNormal );
      sPlacingCubes = true;

      SpawnCube( spawnPos );
    }
  }
} // namespace Tac

