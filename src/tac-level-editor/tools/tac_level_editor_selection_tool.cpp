#include "tac_level_editor_selection_tool.h" // self-inc

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/renderpass/mesh/tac_mesh_presentation.h"
#include "tac-level-editor/selection/tac_level_editor_entity_selection.h"
#include "tac-level-editor/windows/game/tac_level_editor_game_window.h"
#include "tac-level-editor/picking/tac_level_editor_mouse_picking.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/gizmo/tac_level_editor_gizmo_mgr.h"

namespace Tac
{

  static void MousePickingEntityDebug( Ray ray_worldspace,
                                       const World* world,
                                       const Camera* ,
                                       Errors& errors )
  {
    Debug3DDrawData* drawData{ world->mDebug3DDrawData };


    Entity* entity { CreationMousePicking::GetPickedEntity() };
    if( !entity )
      return;


    if( Material * material{ Material::GetMaterial( entity ) } )
      if( !material->mRenderEnabled )
        return;

    Model* model{ Model::GetModel( entity ) };
    if( !model )
      return;

    TAC_CALL( const Mesh* mesh { MeshPresentation::GetModelMesh( model, errors ) } );
    if( !mesh )
      return;

    bool isInverseValid{};
    m4 worldToModel_dir{ m4::Transpose( entity->mWorldTransform ) };
    m4 worldToModel_pos{ m4::Inverse( entity->mWorldTransform, &isInverseValid ) };
    if( !isInverseValid )
      return;

    m4 modelToWorld_dir = m4::Transpose(worldToModel_pos);

    Ray ray_modelspace
    {
      .mOrigin    { ( worldToModel_pos * v4( ray_worldspace.mOrigin, 1 ) ).xyz()},
      .mDirection { ( worldToModel_dir * v4( ray_worldspace.mDirection, 0 ) ).xyz() },
    };

    // TODO(N8):
    //
    // triangle wireframe is an abuse of debug draw!
    // Debug draw uses lines primitives, which rasterizes differently from triangle primitives.
    // 
    // I should use a real mesh triangle wireframe for picked/selected objects
    for( const Triangle& subMeshTri : mesh->mMeshRaycast.mTris )
    {
      const v3 wsTriv0{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 0 ], 1.0f ) ).xyz() };
      const v3 wsTriv1{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 1 ], 1.0f ) ).xyz() };
      const v3 wsTriv2{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 2 ], 1.0f ) ).xyz() };
      const v3 color{ 1, 0, 0 };
      drawData->DebugDraw3DTriangle( wsTriv0, wsTriv1, wsTriv2, color );
    }

    

    if( MeshRaycast::Result closestResult_modelspace{ mesh->mMeshRaycast.Raycast( ray_modelspace ) };
        closestResult_modelspace.IsValid() )
    {
      Triangle closestTri_modelspace{ mesh->mMeshRaycast.mTris[ closestResult_modelspace.mTriIdx ] };
      const v3 triColor{ 0, 1, 0 };

      v3 v0{ ( model->mEntity->mWorldTransform * v4( closestTri_modelspace[ 0 ], 1.0f ) ).xyz() };
      v3 v1{ ( model->mEntity->mWorldTransform * v4( closestTri_modelspace[ 1 ], 1.0f ) ).xyz() };
      v3 v2{ ( model->mEntity->mWorldTransform * v4( closestTri_modelspace[ 2 ], 1.0f ) ).xyz() };

      v3 zfighting = 0.001f * Normalize( Cross( v1 - v0, v2 - v0 ) );
      v0 += zfighting;
      v1 += zfighting;
      v2 += zfighting;

      const v3 centroid{ ( v0 + v1 + v2 ) / 3 };
      v0 += ( centroid - v0 ) * 0.01f;
      v1 += ( centroid - v1 ) * 0.01f;
      v2 += ( centroid - v2 ) * 0.01f;
      for( float t {}; t < 1; t += 0.1f )
      {
        v3 color = v3( 1 - t, 0, 0 );
        drawData->DebugDraw3DTriangle( v0 + ( centroid - v0 ) * t,
                                       v1 + ( centroid - v1 ) * t,
                                       v2 + ( centroid - v2 ) * t,
                                       color );
      }
    }
  }

  SelectionTool SelectionTool::sInstance;
  SelectionTool::SelectionTool()
  {
    mDisplayName = "Selection";
    mIcon = "assets/editor/tools/select_tool.png";
  }
  void SelectionTool::OnToolSelected()
  {
  }
  void SelectionTool::Update()
  {
    if( AppWindowApi::IsHovered( CreationGameWindow::GetWindowHandle() ) )
    {
      if( !GizmoMgr::sInstance.mSelectedGizmo && AppKeyboardApi::JustPressed( Key::MouseLeft ) )
      {
        SelectedEntities::Select( CreationMousePicking::GetPickedEntity() );
      }

      if( CreationMousePicking::sDrawRaycast )
      {
        Camera* camera{ Creation::GetCamera() };
        World* world{ Creation::GetWorld() };
        Ray ray{ CreationMousePicking::GetWorldspaceMouseRay() };
        Errors errors;
        TAC_CALL( MousePickingEntityDebug( ray, world, camera, errors ) );
        TAC_ASSERT( !errors );
      }

      int pickAxis{ -1 };
      for( int i{}; i < 3; ++i )
        if( CreationMousePicking::IsTranslationWidgetPicked( i ) )
          pickAxis = i;

      if( pickAxis != -1 && AppKeyboardApi::JustPressed( Key::MouseLeft ) )
      {
        GizmoMgr* gizmoMgr{ &GizmoMgr::sInstance };
        const v3 worldSpaceHitPoint{ CreationMousePicking::GetPickedPos() };
        const v3 gizmoOrigin{ SelectedEntities::ComputeAveragePosition() };
        const v3 arrowDirs[]{ {1,0,0}, {0,1,0}, {0,0,1} };
        const v3 arrowDir{ arrowDirs[ pickAxis ] };
        gizmoMgr->mSelectedGizmo = true;
        gizmoMgr->mTranslationGizmoDir = arrowDir;
        gizmoMgr->mTranslationGizmoOffset = Dot( arrowDir, worldSpaceHitPoint - gizmoOrigin );
        gizmoMgr->mTranslationGizmoAxis = pickAxis;
      }
    }
  }
} // namespace Tac

