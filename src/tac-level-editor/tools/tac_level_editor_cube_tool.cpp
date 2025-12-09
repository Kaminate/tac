#include "tac_level_editor_cube_tool.h" // self-inc

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-level-editor/windows/game/tac_level_editor_game_window.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/picking/tac_level_editor_mouse_picking.h"

namespace Tac
{
  static float cubeSize{ 1 };

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

  static auto GetEntityPreviewPos(Entity* pickedEntity) -> Optional< v3 >
  {
    Ray mouseRay_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
    Model* model{ Model::GetModel( pickedEntity ) };
    if(!model)
      return {};

    Errors errors;
    const ModelAssetManager::Params getMeshParams
    {
      .mPath       { model->mModelPath },
      .mModelIndex { model->mModelIndex },
    };
    bool isInverseValid{};
    m4 worldToModel_dir{ m4::Transpose( pickedEntity->mWorldTransform ) };
    m4 worldToModel_pos{ m4::Inverse( pickedEntity->mWorldTransform, &isInverseValid ) };
    if( !isInverseValid )
      return {};

    m4 modelToWorld_dir = m4::Transpose(worldToModel_pos);

    Ray mouseRay_modelspace
    {
      .mOrigin    { ( worldToModel_pos * v4( mouseRay_worldspace.mOrigin, 1 ) ).xyz()},
      .mDirection { ( worldToModel_dir * v4( mouseRay_worldspace.mDirection, 0 ) ).xyz() },
    };

    Mesh* mesh{ ModelAssetManager::GetMesh( getMeshParams, errors ) };
    TAC_ASSERT( !errors );
    if( !mesh )
      return {};

    MeshRaycast::Result raycastResult{ mesh->mMeshRaycast.Raycast( mouseRay_modelspace ) };
    if( !raycastResult.IsValid() )
      return {};


    auto closest{ mesh->mMeshRaycast.mTris[ raycastResult.mTriIdx ] };
    const v3 e1{ closest.mVertices[ 1 ] - closest.mVertices[ 0 ] };
    const v3 e2{ closest.mVertices[ 2 ] - closest.mVertices[ 0 ] };
    const v3 n_modelspace{ Normalize( Cross( e1, e2 ) ) };
    const v3 n_worldspace{ ( modelToWorld_dir * v4( n_modelspace, 0 ) ).xyz() };

    v3 hitPos_modelspace{ mouseRay_modelspace.mOrigin + mouseRay_modelspace.mDirection * raycastResult.mT };
    v3 hitPos_worldspace{ ( pickedEntity->mWorldTransform * v4( hitPos_modelspace, 1 ) ).xyz() };

    v3 previewPos_unsnapped = hitPos_worldspace + n_worldspace * cubeSize / 2;

    return Snap( previewPos_unsnapped );
  }

  static auto GetGroundPreviewPos() -> Optional<v3>
  {
    Ray mouseRay_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
    Plane ground{ Plane::FromPosUnitNormal( {}, v3( 0, 1, 0 ) ) };
    RayPlane rayPlane{ mouseRay_worldspace, ground };
    if( !rayPlane )
      return {};
    v3 hitPoint{ rayPlane.mT * mouseRay_worldspace.mDirection + mouseRay_worldspace.mOrigin };
    v3 previewOrigin_unsnapped{ hitPoint + ground.UnitNormal() * cubeSize / 2 };
    return Snap( previewOrigin_unsnapped );
  }

  static auto RandomColor() -> v4
  {
    v3 color{};
    float t{RandomFloat0To1()};
    for( int i{}; i < 3; ++i )
    {
      float a{ v3 {0.8f, 0.5f, 0.4f}[ i ] };
      float b{ v3 {0.2f, 0.4f, 0.2f}[ i ] };
      float c{ v3 {2.0f, 1.0f, 1.0f}[ i ] };
      float d{ v3 {0.0f, 0.2f, 0.2f}[ i ] };
      color[ i ] = a + b * Cos( 6.283185f * ( c * t + d ) );
    }
    return v4( color, 1 );
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
    if( AppWindowApi::IsHovered( CreationGameWindow::GetWindowHandle() ) )
    {
      Ray mouseRay_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };

      // show preview
      World* world{ Creation::GetWorld() };
      Debug3DDrawData* drawData{ world->mDebug3DDrawData };
      
      Optional< v3 > previewPos;
      if( Entity* pickedEntity{CreationMousePicking::GetPickedEntity()} )
        previewPos = GetEntityPreviewPos(pickedEntity);
      else
        previewPos = GetGroundPreviewPos();

      if( previewPos )
      {
        drawData->DebugDraw3DOBB( previewPos.GetValue(), v3(1,1,1) * cubeSize / 2, m3() );
        if( AppKeyboardApi::JustPressed( Key::MouseLeft ) )
        {
          Entity* entity{ Creation::CreateEntity() };
          entity->mRelativeSpace.mPosition = previewPos.GetValue();

          auto model{ ( Model* )entity->AddNewComponent( Model().GetEntry() ) };
          model->mModelPath = "assets/essential/models/box/Box.gltf";
          model->mModelIndex = 0;

          auto material{ ( Material* )entity->AddNewComponent( Material().GetEntry() ) };
          material->mColor = RandomColor();
          material->mShaderGraph = "assets/shader-graphs/gltf_pbr.tac.sg";
          material->mIsGlTF_PBR_MetallicRoughness = true;
          material->mPBR_Factor_Roughness = 1;
        }
      }
    }
  }
} // namespace Tac

