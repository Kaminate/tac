#include "tac_level_editor_mouse_picking.h" // self-inc
#include "tac_level_editor.h"

#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-level-editor/gizmo/tac_level_editor_light_widget.h"
#include "tac-level-editor/selection/tac_level_editor_entity_selection.h"
#include "tac-level-editor/gizmo/tac_level_editor_gizmo_mgr.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-ecs/renderpass/game/tac_game_presentation.h"
#include "tac-ecs/renderpass/mesh/tac_mesh_presentation.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"


namespace Tac
{
  enum class PickedObject
  {
    None = 0,
    Entity,
    WidgetTranslationArrow,
  };

  struct PickData
  {
    bool IsNewClosest( float dist ) const
    {
      return pickedObject == PickedObject::None || dist < closestDist;
    }

    PickedObject pickedObject {};
    float        closestDist  {};
    Entity*      closest      {};
    int          arrowAxis    {};
  };

  struct RaycastResult
  {
    bool  mHit {};
    float mT   {};
  };

  static PickData pickData;
  static v3       mMouseUnitDir_viewspace   {};
  static v3       mMouseUnitDir_worldspace  {};
  static Mesh*    mArrow                    {};
  static bool     mWindowHovered            {};
  bool            CreationMousePicking::sDrawRaycast { true };

  // -----------------------------------------------------------------------------------------------

  // Only need the position for picking
  static auto GetPosOnlyVtxDecls() -> Render::VertexDeclarations
  {
    const Render::VertexDeclaration posDecl
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
    };
    Render::VertexDeclarations m3DvertexFormatDecls;
    m3DvertexFormatDecls.push_back( posDecl );
    return m3DvertexFormatDecls;
  }

  static auto MousePickingEntityLight( const Light* light ) -> RaycastResult
  {
    const Ray ray_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
    const Sphere sphere{ .mOrigin{light->mEntity->mWorldPosition}, .mRadius{LightWidget::sSize} };
    const float t{ RaySphere( ray_worldspace, sphere ) };
    return RaycastResult
    {
      .mHit { t > 0 },
      .mT   { t },
    };
  }

  static auto MousePickingEntityModel( const Model* model, Errors& errors ) -> RaycastResult
  {
    const Ray ray_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
    const Entity* entity { model->mEntity };
    TAC_CALL_RET( const Mesh* mesh { MeshPresentation::GetModelMesh( model, errors ) } );
    if( !mesh )
      return {};

    bool invExists;
    const m4 worldToModel_dir{ m4::Transpose( entity->mWorldTransform ) };
    const m4 worldToModel_pos { m4::Inverse( entity->mWorldTransform, &invExists ) };
    if( !invExists )
      return {};

    const Ray ray_modelspace
    {
      .mOrigin    { ( worldToModel_pos * v4( ray_worldspace.mOrigin, 1 ) ).xyz() },
      .mDirection { ( worldToModel_dir * v4( ray_worldspace.mDirection, 0 ) ).xyz() },
    };

    const MeshRaycast::Result meshRaycastResult { mesh->mMeshRaycast.Raycast( ray_modelspace ) };
    if ( !meshRaycastResult.IsValid() )
      return {};

    const v3 hitPoint_modelspace{ ray_modelspace.mOrigin + ray_modelspace.mDirection * meshRaycastResult.mT };
    const v3 hitPoint_worldspace{ ( entity->mWorldTransform * v4( hitPoint_modelspace, 1 ) ).xyz() };

    return RaycastResult
    {
      .mHit { true },
      .mT   { Distance( ray_worldspace.mOrigin, hitPoint_worldspace ) },
    };
  }

  static auto MousePickingEntity( const Entity* entity, Errors& errors ) -> RaycastResult
  {
    if( const Material * material{ Material::GetMaterial( entity ) } )
      if( !material->mRenderEnabled )
        return {};

    if( const Model * model{ Model::GetModel( entity ) } )
    {
      TAC_CALL_RET( const RaycastResult raycastResult{ MousePickingEntityModel( model, errors ) } );
      if( raycastResult.mHit )
        return raycastResult;
    }

    if( const Light * light{ Light::GetLight( entity ) } )
      if( const RaycastResult raycastResult{ MousePickingEntityLight( light ) };
          raycastResult.mHit )
        return raycastResult;

    return {};
  }

  static void MousePickingGizmos()
  {
    const Camera* camera{ Creation::GetCamera() };
    GizmoMgr* gizmoMgr{ &GizmoMgr::sInstance };
    if( SelectedEntities::empty() || !gizmoMgr->mGizmosEnabled || !mWindowHovered )
      return;

    const v3 selectionGizmoOrigin { gizmoMgr->mGizmoOrigin };
    const m4 invArrowRots[]{ m4::RotRadZ( 3.14f / 2.0f ),
                             m4::Identity(),
                             m4::RotRadX( -3.14f / 2.0f ), };
    for( int i{}; i < 3; ++i )
    {
      // 1/3: inverse transform
      v3 modelSpaceRayPos3 { camera->mPos - selectionGizmoOrigin };
      v4 modelSpaceRayPos4 { v4( modelSpaceRayPos3, 1 ) };
      v3 modelSpaceRayDir3 { mMouseUnitDir_worldspace };
      v4 modelSpaceRayDir4 { v4( mMouseUnitDir_worldspace, 0 ) };

      // 2/3: inverse rotate
      const m4& invArrowRot { invArrowRots[ i ] };
      modelSpaceRayPos4 = invArrowRot * modelSpaceRayPos4;
      modelSpaceRayPos3 = modelSpaceRayPos4.xyz();
      modelSpaceRayDir4 = invArrowRot * modelSpaceRayDir4;
      modelSpaceRayDir3 = modelSpaceRayDir4.xyz();

      // 3/3: inverse scale
      modelSpaceRayPos3 /= gizmoMgr-> mArrowLen;

      const Ray meshRay
      { 
        .mOrigin{ modelSpaceRayPos3 },
        .mDirection{ modelSpaceRayDir3 },
      };

      const MeshRaycast::Result meshRaycastResult{ mArrow->mMeshRaycast.Raycast( meshRay ) };
      if( !meshRaycastResult.IsValid()  )
        continue;

      const float dist{ meshRaycastResult.mT * gizmoMgr->mArrowLen };
      if( !pickData.IsNewClosest( dist ) )
        continue;

      pickData.arrowAxis = i;
      pickData.closestDist = dist;
      pickData.pickedObject = PickedObject::WidgetTranslationArrow;
    }
  }

  static void MousePickingEntities( Errors& errors )
  {
    if( mWindowHovered )
    {
      for( World* world{ Creation::GetWorld() };
           Entity* entity : world->mEntities )
      {
        TAC_CALL( const RaycastResult raycastResult{ MousePickingEntity( entity, errors ) } );
        if( !raycastResult.mHit || !pickData.IsNewClosest( raycastResult.mT ) )
          continue;

        pickData.closestDist = raycastResult.mT;
        pickData.closest = entity;
        pickData.pickedObject = PickedObject::Entity;
      }
    }

  }

  static void MousePickingSelection()
  {
    if( !AppKeyboardApi::JustPressed( Key::MouseLeft ) || !mWindowHovered )
      return;


    switch( pickData.pickedObject )
    {
      case PickedObject::WidgetTranslationArrow:
      {
        GizmoMgr* gizmoMgr{ &GizmoMgr::sInstance };
        Camera* camera{ Creation::GetCamera() };
        const v3 worldSpaceHitPoint{ camera->mPos + pickData.closestDist * mMouseUnitDir_worldspace };
        const v3 gizmoOrigin{ SelectedEntities::ComputeAveragePosition() };

        v3 arrowDir{};
        arrowDir[ pickData.arrowAxis ] = 1;

        gizmoMgr->mSelectedGizmo = true;
        gizmoMgr->mTranslationGizmoDir = arrowDir;
        gizmoMgr->mTranslationGizmoOffset = Dot( arrowDir, worldSpaceHitPoint - gizmoOrigin );
        gizmoMgr->mTranslationGizmoAxis = pickData.arrowAxis;
      } break;

      case PickedObject::Entity:
      {
        // Commenting out so that this is handled in the new tac_level_editor_selection_tool.h
        //SelectedEntities::Select( pickData.closest );
      } break;

      case PickedObject::None:
      {
        // Commenting out so that this is handled in the new tac_level_editor_selection_tool.h
        //SelectedEntities::clear();
      } break;
    }
  }

  auto CreationMousePicking::GetPickedEntity() -> Entity*
  {
    return pickData.pickedObject == PickedObject::Entity ? pickData.closest : nullptr;
  }

  auto CreationMousePicking::GetPickedPos() -> v3
  {
    Ray ray{ GetWorldspaceMouseRay() };
    return ray.mOrigin + ray.mDirection * pickData.closestDist;
  }

  void CreationMousePicking::Init( Errors& errors )
  {
    const Render::VertexDeclarations m3DvertexFormatDecls{ GetPosOnlyVtxDecls() };
    const ModelAssetManager::Params meshParams
    {
      .mPath        { "assets/editor/arrow.gltf" },
      .mOptVtxDecls { m3DvertexFormatDecls },
    };
    TAC_CALL( mArrow = ModelAssetManager::GetMesh( meshParams, errors ) );
  }

  auto CreationMousePicking::GetWorldspaceMouseDir() -> v3{ return mMouseUnitDir_worldspace; }

  bool CreationMousePicking::IsTranslationWidgetPicked( int i )
  {
    return
      pickData.pickedObject == PickedObject::WidgetTranslationArrow &&
      pickData.arrowAxis == i;
  }

  void CreationMousePicking::Update( Errors& errors )
  {
    pickData = {};
    TAC_CALL( MousePickingEntities( errors ) );
    MousePickingGizmos();
    MousePickingSelection();
    if( pickData.pickedObject != PickedObject::None )
    {
      dynmc World* world{ Creation::GetWorld() };
      const Camera* camera{ Creation::GetCamera() };
      const v3 worldSpaceHitPoint{ camera->mPos + pickData.closestDist * mMouseUnitDir_worldspace };
      world->mDebug3DDrawData->DebugDraw3DSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );
    }
  }

  void CreationMousePicking::BeginFrame( WindowHandle mWindowHandle )
  {
    mWindowHovered = AppWindowApi::IsHovered( mWindowHandle );
    const Camera* camera{ Creation::GetCamera() };
    const v2i windowSize{ AppWindowApi::GetSize( mWindowHandle ) };
    const v2i windowPos{ AppWindowApi::GetPos( mWindowHandle ) };
    const float w{ ( float )windowSize.x };
    const float h{ ( float )windowSize.y };
    const float x{ ( float )windowPos.x };
    const float y{ ( float )windowPos.y };
    const v2 screenspaceCursorPos { AppKeyboardApi::GetMousePosScreenspace() };
    dynmc float xNDC { ( ( screenspaceCursorPos.x - x ) / w ) };
    dynmc float yNDC { ( ( screenspaceCursorPos.y - y ) / h ) };
    yNDC = 1 - yNDC;
    xNDC = xNDC * 2 - 1;
    yNDC = yNDC * 2 - 1;
    const float aspect { w / h };
    const float theta { camera->mFovyrad / 2.0f };
    const float cotTheta { 1.0f / Tan( theta ) };
    const float sX { cotTheta / aspect };
    const float sY { cotTheta };
    const m4 viewInv{ m4::ViewInv( camera->mPos,
                                   camera->mForwards,
                                   camera->mRight,
                                   camera->mUp ) };
    const v3 viewSpaceMousePosNearPlane{ xNDC / sX,
                                         yNDC / sY,
                                         -1 };
    const v3 viewSpaceMouseDir { Normalize( viewSpaceMousePosNearPlane ) };
    const v4 viewSpaceMouseDir4 { v4( viewSpaceMouseDir, 0 ) };
    const v4 worldSpaceMouseDir4 { viewInv * viewSpaceMouseDir4 };
    mMouseUnitDir_worldspace = worldSpaceMouseDir4.xyz();
    mMouseUnitDir_viewspace = viewSpaceMouseDir;
  }

  auto CreationMousePicking::GetWorldspaceMouseRay() -> Ray
  {
    const Camera* camera{ Creation::GetCamera() };
    return Ray
    {
      .mOrigin    { camera->mPos },
      .mDirection { mMouseUnitDir_worldspace },
    };
  }


} // namespace Tac

